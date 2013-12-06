<?php
 /*  
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

class Locale
{
        public static $code;
        public static $translations;

        public static function init($visitor)
        {
                global $base_dir;

                self::$code = "en";

                if (isset($visitor)) {
                        //echo "locale: visitor_account";
                        //var_dump($visitor);
                        self::$code = $visitor->locale;

                } else if (isset($_REQUEST["locale"])) {
                        //echo "locale: request";
                        self::$code = $_REQUEST["locale"];
                        $_SESSION["locale"] = self::$code;

                } else if (isset($_SESSION["locale"])) {
                        //echo "locale: session";
                        self::$code = $_SESSION["locale"];

                } else if (isset($_SERVER['HTTP_ACCEPT_LANGUAGE'])) {

                        //echo "locale: HTTP_ACCEPT_LANGUAGE: " . $_SERVER['HTTP_ACCEPT_LANGUAGE'];

                        // From Jesse Skinner's blog
                        // http://www.thefutureoftheweb.com/blog/use-accept-language-header
                        $langs = array();

                
                        // break up string into pieces (languages and q factors)
                        preg_match_all('/([a-z]{1,8}(-[a-z]{1,8})?)\s*(;\s*q\s*=\s*(1|0\.[0-9]+))?/i', 
                                       $_SERVER['HTTP_ACCEPT_LANGUAGE'], 
                                       $lang_parse);
        
                        if (count($lang_parse[1])) {
                                // create a list like "en" => 0.8
                                $langs = array_combine($lang_parse[1], $lang_parse[4]);
                
                                // set default to 1 for any without q factor
                                foreach ($langs as $lang => $val) {
                                        if ($val === '') $langs[$lang] = 1;
                                }
                        
                                // sort list based on value	
                                arsort($langs, SORT_NUMERIC);
                        }

                        // look through sorted list and use first one that matches our languages
                        foreach ($langs as $lang => $val) {
                                //echo "($lang, $val):" . strpos($lang, 'fr') . ", ";
                                if (strpos($lang, 'fr') === 0) {
                                        self::$code = "fr";
                                        break;
                                } else if (strpos($lang, 'en') === 0) {
                                        self::$code = "en";
                                        break;
                                } 
                        }
                }

                //echo "locale: self::$code";

                global $base_dir;

                $path = "locale/" .  self::$code . ".json";
                $json = file_get_contents($path);
                self::$translations = json_decode($json);

                if (!self::$translations) {

                        $msg = "";
                        switch (json_last_error()) {
                        case JSON_ERROR_NONE:
                                $msg = 'The JSON decoded returned an empty object';
                                break;
                        case JSON_ERROR_DEPTH:
                                $msg = 'JSON decoder: Maximum stack depth exceeded';
                                break;
                        case JSON_ERROR_STATE_MISMATCH:
                                $msg = 'JSON decoder: Underflow or the modes mismatch';
                                break;
                        case JSON_ERROR_CTRL_CHAR:
                                $msg = 'JSON decoder: Unexpected control character found';
                                break;
                        case JSON_ERROR_SYNTAX:
                                $msg = 'JSON decoder: Syntax error, malformed JSON';
                                break;
                        case JSON_ERROR_UTF8:
                                $msg = 'JSON decoder: Malformed UTF-8 characters, possibly incorrectly encoded';
                                break;
                        default:
                                $msg = 'JSON decoder: Unknown error';
                                break;
                        }
                        echo "LOCALE: $msg<br/>\n";    
                        echo "LOCALE: lang: '" . self::$code . "'<br/>\n";    
                        echo "LOCALE: path: $path<br/>\n";    
                }

                //echo $path;
                //echo $json;
                //var_dump($translations);
        }

        public static function translate($s) {
                if (self::$translations 
                    && property_exists(self::$translations, $s))
                        return self::$translations->$s;                
                else return $s;
        }
}

function _s($s) 
{
        return Locale::translate($s);
}

function _p($s) 
{
        echo Locale::translate($s);
}

class Language
{
        public $code;
        public $group;
        public $english;
        public $native;
        public $interface;

        public function import($row) 
        {
                $this->code = $row['code'];
                $this->group = $row['group'];
                $this->english = $row['english'];
                $this->native = $row['native'];
                $this->interface = $row['interface'];
        }

        public function load($code) 
        {
                global $mysqli;
                
                $query = ("SELECT * FROM locale WHERE "
                          . "code='" . $mysqli->real_escape_string($code) . "'");
                //echo $query . "<br/>\n";

                $res = $mysqli->query($query);
                if ($mysqli->errno) {
                        //echo $mysqli->error . "<br/>\n";
                        return false;
                }
                if (!$res || ($res->num_rows != 1)) return false;
                $row = $res->fetch_assoc();
                $this->import($row);

                return true;
        }        
}

function get_languages()
{
        global $mysqli;
        
        $query = ("SELECT * FROM locale ORDER by `english`");
        //echo $query . "<br/>\n";

        $res = $mysqli->query($query);
        if ($mysqli->errno) {
                //echo $mysqli->error . "<br/>\n";
                return FALSE;
        }

        $lang = array();

        while ($row = $res->fetch_assoc()) {
                $obj = new Language();
                $obj->import($row);
                $lang[] = $obj;
        }
        
        return $lang;
}

function get_most_used_languages()
{
        global $mysqli;
        
        $query = ("SELECT * FROM locale WHERE most_used=1 ORDER by `english`");
        //echo $query . "<br/>\n";

        $res = $mysqli->query($query);
        if ($mysqli->errno) {
                //echo $mysqli->error . "<br/>\n";
                return FALSE;
        }

        $lang = array();

        while ($row = $res->fetch_assoc()) {
                $obj = new Language();
                $obj->import($row);
                $lang[] = $obj;
        }
        
        return $lang;
}

function get_interface_languages()
{
        global $mysqli;
        
        $query = ("SELECT * FROM locale WHERE `interface`=1 ORDER by `native`");
        //echo $query . "<br/>\n";

        $res = $mysqli->query($query);
        if ($mysqli->errno) {
                //echo $mysqli->error . "<br/>\n";
                return FALSE;
        }

        $lang = array();

        while ($row = $res->fetch_assoc()) {
                $obj = new Language();
                $obj->import($row);
                $lang[] = $obj;
        }
        
        return $lang;
}

?>
