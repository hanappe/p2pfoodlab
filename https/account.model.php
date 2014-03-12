<?php /* 

    P2P Food Lab  

    P2P Food Lab is an open, collaborative platform to develop an
    alternative food value system.

    Copyright (C) 2013  Sony Computer Science Laboratory Paris
    Author: Peter Hanappe

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

require_once "opensensordata.inc.php";

function visitor_at_home($host, $visitor)
{
        return ($visitor_account 
                && $host_account 
                && ($host_account->id == $visitor_account->id));
}

function visitor_registered($visitor)
{
        return $visitor_account != NULL; 
}

function load_visitor_account()
{
        $account = NULL;

        if (isset($_SESSION['account'])) {
                if (!isset($_SESSION['timeout'])) {
                        unset($_SESSION['account']);

                } else if ($_SESSION['timeout'] + 3600 < time()) {
                        unset($_SESSION['account']);

                } else {
                        $account = $_SESSION['account'];
                        $_SESSION['timeout'] = time();
                }
        }
        return $account;
}

function load_host_account()
{
        $account = NULL;

        $host = input_get_host();
        if (is_valid_username($host)) {
                $account = new Account();
                if (!$account->load("username", $host)) {
                        badRequest("Invalid username: " . $host);
                }
        }
        return $account;
}

function do_login()
{
        global $page;

        $name = input_get_visitor();
        $pwhash = input_get_pwhash();

        if (!is_valid_pwhash($pwhash)
            || !is_valid_username($name)) {
                $page = new LoginPage($name);
                $page->message = "Login failed";
                $page->generate();
        }
        
        $account = new Account();
        if (!$account->load("username", $name)) {
                $page = new LoginPage($name);
                $page->message = "Login failed";
                $page->generate();
        }
        
        if (!$account->validated) {
                $page = new LoginPage($name);
                $page->message = "Please validate your account before connecting to the site.";
                $page->generate();
        }

        if ($account->pwhash != $pwhash) {
                $page = new LoginPage($name);
                $page->message = "Login failed";
                $page->generate();
        }
        
        $_SESSION['account'] = $account;
        $_SESSION['timeout'] = time();

        return $account;
}

function create_account_key()
{
        // Generate an UUID, sort of...
        return sprintf('%04x%04x-%04x-%04x-%04x-%04x%04x%04x',
                       // 32 bits for "time_low"
                       mt_rand(0, 0xffff), mt_rand(0, 0xffff),
                       
                       // 16 bits for "time_mid"
                       mt_rand(0, 0xffff),
                       
                       // 16 bits for "time_hi_and_version",
                       // four most significant bits holds version number 4
                       mt_rand(0, 0x0fff) | 0x4000,
                       
                       // 16 bits, 8 bits for "clk_seq_hi_res",
                       // 8 bits for "clk_seq_low",
                       // two most significant bits holds zero and one for variant DCE1.1
                       mt_rand(0, 0x3fff) | 0x8000,
                       
                       // 48 bits for "node"
                       mt_rand(0, 0xffff), mt_rand(0, 0xffff), mt_rand(0, 0xffff));
}

function create_account()
{
        global $base_url, $page, $contact_address;

        $username = input_get_username();
        $email = input_get_email();
        $pw = input_get_password();
        $agree = $_POST['agree'];
        $locale = input_get_locale();

        if ($agree != 'yes') {
                $page = new CreateAccountForm($username, $email, $pw, $agree, $locale);
                $page->message = _s("You must accept the Open Database License to create an account on P2P Food Lab");
                $page->generate();
        }
        if (!is_valid_username($username)) {
                $page = new CreateAccountForm($username, $email, $pw, $agree, $locale);
                $page->message = _s("Please enter a valid account name (min. 3 characters, starts with a letter, digits accepted): ") . $username;
                $page->generate();
        }
        if (!is_valid_email($email)) {
                $page = new CreateAccountForm($username, $email, $pw, $agree, $locale);
                $page->message = _s("Please verify your email address. It failed to validate.");
                $page->generate();
        }
        if (!is_valid_password($pw)) {
                $page = new CreateAccountForm($username, $email, $pw, $agree, $locale);
                $page->message = _s("Please verify your password. It failed to validate.");
                $page->generate();
        }

        if (!is_valid_locale($locale)) {
                $page = new CreateAccountForm($username, $email, $pw, $agree, $locale);
                $page->message = _s("Invalid interface language.");
                $page->generate();
        }

        $securimage = new Securimage();
        if ($securimage->check($_POST['captcha_code']) == false) {
                $page = new CreateAccountForm($username, $email, $pw, $agree, $locale);
                $page->message = _s("The security code you entered was not correct. Please try again.");
                $page->generate();
        }

        $account = new Account();

        // Check for an existing account
        if ($account->load("username", $username)) {
                $page = new CreateAccountForm($username, $email, $pw, $agree, $locale);
                $page->message = _s("The username you entered is already used. Please select another one. Sorry...");
                $page->generate();
        }

        $account->username = $username;
        $account->email = $email;
        $account->pwhash = md5($username . $pw);
        $account->key = create_account_key();
        $account->validated = 0;
        $account->code = create_account_key();
        $account->locale = $locale;

        if (!$account->create()) 
                internalServerError("Create: Database error: " . $account->err);

        if (!$account->set_lang($locale, true)) 
                internalServerError("Create: Database error: " . $account->err);

        $subject = 'Welcome to P2P Food Lab!';
        $message = 
"Welcome to P2P Food Lab!

To validate your account, please open the following link: 
" . $base_url . "/validate/" . $account->code . "

Cheers!
";

        $headers = ('From: ' . $contact_address . "\r\n" .
                    'Reply-To: ' . $contact_address);

        mail($account->email, $subject, $message, $headers);


        $subject = '[P2P Food Lab] New account';
        $message = "email " . $account->email . ", username '" . $account->username . "'";
        $headers = ('From: ' . $contact_address . "\r\n");
        mail($contact_address, $subject, $message, $headers);

        return $account;
 }

function validate_account()
{
        global $contact_address;

        $code = input_get('code');
        if (!is_valid_key($code))
                badRequest("Invalid code: " . $code);

        $account = new Account();
        if (!$account->load("code", $code)) 
                internalServerError("Failed to find the account associated with the code " 
                                    . $code);

        if (!$account->validated) {
                if (!$account->validate()) 
                        internalServerError($account->err);
                
                $subject = '[P2P Food Lab] Account validated';
                $message = "email " . $account->email . ", username '" . $account->username . "'";
                $headers = ('From: ' . $contact_address . "\r\n");
                mail($contact_address, $subject, $message, $headers);
                
                db_close();

                $_SESSION['account'] = $account;
                $_SESSION['timeout'] = time();
        }

        return $account;
  }

class Account
{
        public $id;
        public $username;
        public $email;
        public $pwhash;
        public $key;
        public $validated;
        public $code;
        public $locale;
        public $osd_id;
        public $osd_username;
        public $osd_key;
        public $err;

        public static function load_all() 
        {
                global $mysqli;

                $query = ("SELECT `id`, `username`, `email`, `pwhash`, `key`, `validated`, "
                          . "`code`, `locale`, `osd_id`, `osd_username` , `osd_key` "
                          . "FROM accounts WHERE 1 ORDER BY username");
                
                //echo $query . "<br/>\n";

                $res = $mysqli->query($query);
                if ($mysqli->errno) {
                        $this->err = $mysqli->error;
                        return false;
                }

                $accounts = array();

                while ($row = $res->fetch_assoc()) {
                        $account = new Account();
                        $account->import($row);
                        if ($account->load_lang() == false)
                                return false;
                        $accounts[] = $account;
                }

                return $accounts;
        }

        public function import($row) 
        {
                $this->id = $row['id'];
                $this->username = $row['username'];
                $this->email = $row['email'];
                $this->pwhash = $row['pwhash'];
                $this->key = $row['key'];
                $this->validated = $row['validated'];
                $this->code = $row['code'];
                $this->locale = $row['locale'];
                $this->osd_id = $row['osd_id'];
                $this->osd_username = $row['osd_username'];
                $this->osd_key = $row['osd_key'];
        }

        public function load($field, $value) 
        {
                global $mysqli;

                $query = ("SELECT `id`, `username`, `email`, `pwhash`, `key`, `validated`, "
                          . "`code`, `locale`, `osd_id`, `osd_username`, `osd_key` "
                          . "FROM accounts WHERE " 
                          . "`" . $field . "`='" . $mysqli->real_escape_string($value) . "'");
                
                //echo $query . "<br/>\n";

                $res = $mysqli->query($query);
                if ($mysqli->errno) {
                        $this->err = $mysqli->error;
                        return false;
                }
                if (!$res || ($res->num_rows != 1)) return false;
                $row = $res->fetch_assoc();

                $this->import($row);
                //var_dump($this);

                if ($this->load_lang() == false)
                        return false;

                return true;
        }

        public function load_lang() 
        {
                global $mysqli;

                $this->lang = array();

                $query = ("SELECT `lang` FROM account_lang "
                          . "WHERE `id`=" . $this->id . " ORDER by `order`");
                
                //echo $query . "<br/>\n";

                $res = $mysqli->query($query);
                if ($mysqli->errno) {
                        $this->err = $mysqli->error;
                        return false;
                }

                while ($row = $res->fetch_assoc()) {
                        $this->lang[] = $row['lang'];
                }

                return true;
        }

        public function load_sensorboxes() 
        {
                global $mysqli;

                $this->sensorbox = array();

                $query = ("SELECT `group_id` FROM account_sensorbox "
                          . "WHERE `account_id`=" . $this->id);
                //echo $query . "<br/>\n";

                $res = $mysqli->query($query);
                if ($mysqli->errno) {
                        $this->err = $mysqli->error;
                        return false;
                }

                while ($row = $res->fetch_assoc()) {
                        $this->sensorbox[] = $row['group_id'];
                }

                return true;                
        }
        
        public function create() 
        {
                global $mysqli;
                
                if (isset($this->id)) 
                        return false;

                $query = ("INSERT INTO accounts VALUES ("
                          . "NULL,"
                          . "'" . $mysqli->real_escape_string($this->username) . "',"
                          . "'" . $mysqli->real_escape_string($this->email) . "',"
                          . "'" . $mysqli->real_escape_string($this->pwhash) . "',"
                          . "'" . $mysqli->real_escape_string($this->key) . "',"
                          . $mysqli->real_escape_string($this->validated) . ","
                          . "'" . $mysqli->real_escape_string($this->code) . "',"
                          . "'" . $mysqli->real_escape_string($this->locale) . "',"
                          . "NULL,NULL,NULL)"
                          );

                //echo $query . "<br/>\n";
                
                $res = $mysqli->query($query);
                if ($mysqli->errno) {
                        $this->err = $mysqli->error;
                        return false;
                }

                $this->id = $mysqli->insert_id;
                return true;
        }

        public function set_locale($locale) 
        {
                global $mysqli;
                
                $this->locale = $locale;

                $query = ("UPDATE accounts SET "
                          . "locale='" . $mysqli->real_escape_string($this->locale) . "' "
                          . "WHERE id=" . $this->id . ";");
                //echo $query . "<br/>\n";

                $res = $mysqli->query($query);
                if ($mysqli->errno) {
                        $this->err = $mysqli->error;
                        return false;
                }

                return true;
        }

        public function set_lang($lang, $select) 
        {
                global $mysqli;
                
                if ($select) {
                        $query = ("REPLACE INTO account_lang VALUES ("
                                  . $mysqli->real_escape_string($this->id) . ","
                                  . "'" . $mysqli->real_escape_string($lang) . "',"
                                  . "0);");
                        //echo $query . "<br/>\n";

                        $res = $mysqli->query($query);
                        if ($mysqli->errno) {
                                $this->err = $mysqli->error;
                                return false;
                        }
                } else {
                        $query = ("DELETE FROM account_lang WHERE "
                                  . "id=" . $mysqli->real_escape_string($this->id) 
                                  . " AND "
                                  . "lang='" . $mysqli->real_escape_string($lang) . "';");

                        //echo $query . "<br/>\n";

                        $res = $mysqli->query($query);
                        if ($mysqli->errno) {
                                $this->err = $mysqli->error;
                                return false;
                        }
                }

                if ($this->load_lang() == false)
                        return false;

                return true;
        }

        public function set_sensorbox($gid, $select) 
        {
                global $mysqli;
                
                if ($select) {
                        $query = ("REPLACE INTO account_sensorbox VALUES ("
                                  . $this->id . "," . $gid . ");");
                        //echo $query . "<br/>\n";

                        $res = $mysqli->query($query);
                        if ($mysqli->errno) {
                                $this->err = $mysqli->error;
                                return false;
                        }
                } else {
                        $query = ("DELETE FROM account_sensorbox WHERE "
                                  . "account_id=" . $this->id 
                                  . " AND "
                                  . "group_id=" . $gid . ";");

                        //echo $query . "<br/>\n";

                        $res = $mysqli->query($query);
                        if ($mysqli->errno) {
                                $this->err = $mysqli->error;
                                return false;
                        }
                }
                return true;
        }

        public function set_datastream($gid, $did, $select) 
        {
                global $mysqli;
                
                if ($select) {
                        $query = ("REPLACE INTO sensorbox_datastreams VALUES ("
                                  . $gid . "," . $did . ");");
                        //echo $query . "<br/>\n";

                        $res = $mysqli->query($query);
                        if ($mysqli->errno) {
                                $this->err = $mysqli->error;
                                return false;
                        }
                } else {
                        $query = ("DELETE FROM sensorbox_datastreams WHERE "
                                  . "group_id=" . $gid 
                                  . " AND "
                                  . "datastream_id=" . $did . ";");

                        //echo $query . "<br/>\n";

                        $res = $mysqli->query($query);
                        if ($mysqli->errno) {
                                $this->err = $mysqli->error;
                                return false;
                        }
                }
                return true;
        }

        public function validate() 
        {
                global $mysqli, $osd_appkey;
                
                $osd = new OpenSensorData(null, null);
                $osd->appkey = $osd_appkey;
                $osd_account = $osd->create_account("p2pfl_" . $this->username, $this->email);
                if (!$osd_account) {
                        $this->err = "Failed to create the OpenSensorData.net account: " . $osd->msg;
                        return false;
                }

                $query = ("UPDATE accounts SET "
                          . "validated=1, "
                          . "osd_id='" . $mysqli->real_escape_string($osd_account->id) . "', "
                          . "osd_username='" . $mysqli->real_escape_string($osd_account->username) . "', "
                          . "osd_key='" . $mysqli->real_escape_string($osd_account->key) . "' "
                          . "WHERE id=" . $this->id . ";");
                //echo $query . "<br/>\n";

                $res = $mysqli->query($query);
                if ($mysqli->errno) {
                        $this->err = $mysqli->error;
                        return false;
                }

                $this->osd_id = $osd_account->id;
                $this->osd_username = $osd_account->username;
                $this->osd_key = $osd_account->key;

                return true;
        }

        public function supports_lang($s) 
        {
                return in_array($s, $this->lang);
        }

        public function count_messages() 
        {
                global $mysqli;

                if (isset($this->message_count))
                        return $this->message_count;

                $query = ("SELECT COUNT(*) as count FROM posts WHERE `from`=" . $this->id);
                //echo $query . "<br/>\n";

                $res = $mysqli->query($query);
                if ($mysqli->errno) {
                        $this->err = $mysqli->error;
                        return false;
                }
                if (!$res || ($res->num_rows != 1)) {
                        $this->err = "SQL query failed";
                        return false;
                }
                $row = $res->fetch_assoc();
                $this->message_count = $row['count'];

                return $this->message_count;
        }
}
?>
