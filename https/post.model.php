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

require_once "doc.model.php";
require_once "thread.model.php";

function valid_symbol_char($c) 
{
        return (strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-_", $c) != NULL);
}

class Post
{
        public $id;
        public $from;
        public $thread_id;
        public $replyto;
        public $date;
        public $text;
        public $text_html;
        public $lang;
        public $thread;

        public $tags;
        public $channels;
        public $accounts;
        public $docs;

        public $err = "";

        public static function sanitise_text($text)
        {
                if (strlen($text) > 2000) {
                        $text = substr($text, 0, 2000);
                }
                return $text;
        }

        public function import($row) 
        {
                $this->id = $row['id'];
                $this->from = $row['from'];
                $this->thread_id = $row['thread'];
                $this->replyto = $row['replyto'];
                $this->date = $row['date'];
                $this->text = $row['text'];
                $this->text_html = $row['text_html'];
                $this->lang = $row['lang'];
        }

        public function load($field, $value) 
        {
                global $mysqli;

                $query = ("SELECT `id`, `from`, `thread`, `replyto`, `date`, `text`, `text_html`, `lang` FROM posts WHERE " 
                          . "`" . $field . "`='" . $mysqli->real_escape_string($value) . "'");

                //echo $query . "<br/>\n";

                $res = $mysqli->query($query);
                if (!$res || ($res->num_rows != 1)) return false;
                $row = $res->fetch_assoc();

                $this->import($row);

                return true;
        }

        public function load_docs() 
        {
                global $mysqli;
                
                $query = ("SELECT doc.id AS `id` , doc.path AS `path` , doc.type AS `type` "
                          . "FROM docs doc, post_docs x " 
                          . "WHERE x.post_id = " . $this->id . " "
                          . "AND x.doc_id = doc.id");
                
                //echo $query . "<br>\n";

                $docs = array();

                $res = $mysqli->query($query);
                if ($mysqli->errno) {
                        $this->err = $mysqli->error;
                        return false;
                }

                while ($row = $res->fetch_assoc()) {
                        $doc = new Doc();
                        $doc->import($row);
                        $docs[] = $doc;
                        //echo $this->id . " -> " . $doc->section_id . "<br/>\n";
                }

                $this->docs = $docs;

                return true;
        }

        public function load_thread() 
        {
                $thread = new Thread();
                $id = $this->thread_id? $this->thread_id : $this->id;
                if (!$thread->load($id)) {
                        return false;
                }
                $this->thread = $thread;
        }

        public function load_channels() 
        {
                global $mysqli;
                
                $this->channels = array();
                
                $query = ("SELECT `channel_id` as `id` FROM `channel_posts` WHERE " 
                          . "`post_id`='" . $mysqli->real_escape_string($this->id) . "'");
                
                //echo $query . "<br/>\n";
                
                $res = $mysqli->query($query);
                if ($mysqli->errno) {
                        $this->err = $mysqli->error;
                        return false;
                }

                while ($row = $res->fetch_assoc()) {
                        //echo $row['id'] . "<br/>\n";
                        $this->channels[] = $row['id'];
                }

                return true;
        }

        public function in_channel($id) 
        {
                for ($i = 0; $i < count($this->channels); $i++) {
                        if ($this->channels[$i] == $id)
                                return true;
                }
                return false;
        }

        public function add_doc($doc) 
        {
                global $mysqli;

                $query .= ("INSERT INTO post_docs VALUES (" . $this->id . "," . $doc->id . "); ");
                echo $query . "<br/>\n";

                $res = $mysqli->query($query);
                if ($mysqli->errno) {
                        $this->err = $mysqli->error;
                        return false;
                }

                if (!isset($this->docs)) {
                        $this->docs = array();
                }
                $this->docs[] = $doc;

                return true;
        }

        public function remove_doc($id) 
        {
                global $mysqli;

                $query = ("DELETE FROM post_docs WHERE "
                          . "post_id=" . $this->id . " AND " 
                          . "doc_id=" . $id . ";\n" );
                //echo $query . "<br/>\n";
                $mysqli->query($query);
                if ($mysqli->errno) {
                        $this->err = $mysqli->error;
                        return false;
                }

                return true;
        }
        
        public function store() 
        {
                global $mysqli;
                
                if (isset($this->id)) 
                        return false;

                $query = ("INSERT INTO posts VALUES ("
                          . "NULL,"
                          . "'" . $mysqli->real_escape_string($this->from) . "',"
                          . "'" . $mysqli->real_escape_string($this->thread_id) . "',"
                          . "'" . $mysqli->real_escape_string($this->replyto) . "',"
                          . "'" . $mysqli->real_escape_string($this->date) . "',"
                          . "'" . $mysqli->real_escape_string($this->text) . "',"
                          . "'" . $mysqli->real_escape_string($this->text_html) . "',"
                          . "'" . $mysqli->real_escape_string($this->lang) . "')");

                //echo $query . "<br/>\n";
                
                $res = $mysqli->query($query);
                if ($mysqli->errno) {
                        $this->err = $mysqli->error;
                        return false;
                }

                $this->id = $mysqli->insert_id;

                $this->post_to_account($this->from);
                $this->store_links();

                return true;
        }

        public function post_to_account($id) 
        {
                global $mysqli;

                $query = ("INSERT INTO account_posts VALUES ("
                          . $id . ","
                          . "" . $this->id . ","
                          . "'" . $mysqli->real_escape_string($this->lang) . "')");
                //echo $query . "<br/>\n";
                $res = $mysqli->query($query);
                if ($mysqli->errno) {
                        $this->err = $mysqli->error;
                        return false;
                }
                return true;
        }

        public function store_links() 
        {
                global $mysqli;

                if (count($this->tags) > 0) {
                        $query = "INSERT INTO tag_posts VALUES ";
                        for ($i = 0; $i < count($this->tags); $i++) {
                                $tag = $this->tags[$i];
                                if ($i > 0) $query .= ",";
                                $query .= "(" . $tag->id . "," . $this->id . ")";
                        }
                        //echo $query . "<br/>\n";
                        $res = $mysqli->query($query);
                        if ($mysqli->errno) {
                                $this->err = $mysqli->error;
                                return false;
                        }
                }

                if (count($this->channels) > 0) {
                        $query = "INSERT INTO channel_posts VALUES ";
                        for ($i = 0; $i < count($this->channels); $i++) {
                                if ($i > 0) $query .= ",";
                                $query .= 
                                        ("("
                                         . "" . $mysqli->real_escape_string($this->channels[$i]) . ","
                                         . "" . $mysqli->real_escape_string($this->id) . ","
                                         . "'" . $mysqli->real_escape_string($this->lang) . "')");
                        }
                        //echo $query . "<br/>\n";
                        $res = $mysqli->query($query);
                        if ($mysqli->errno) {
                                $this->err = $mysqli->error;
                                return false;
                        }
                }

                if (count($this->docs) > 0) {
                        $query = "INSERT INTO post_docs VALUES ";
                        for ($i = 0; $i < count($this->docs); $i++) {
                                if ($i > 0) $query .= ",";
                                $doc = $this->docs[$i];
                                $query .= "(" . $this->id . "," . $doc->id . ")";
                        }
                        echo $query . "<br/>\n";
                        $res = $mysqli->query($query);
                        if ($mysqli->errno) {
                                $this->err = $mysqli->error;
                                return false;
                        }
                }

                for ($i = 0; $i < count($this->accounts); $i++) {
                        if (!$this->post_to_account($this->accounts[$i]))
                                return false;
                }
        }

        public function update() 
        {
                global $mysqli;
                
                $query = ("UPDATE posts SET "
                          . "text='" . $mysqli->real_escape_string($this->text) . "', "
                          . "text_html='" . $mysqli->real_escape_string($this->text_html) . "', "
                          . "lang='" . $mysqli->real_escape_string($this->lang) . "' "
                          . "WHERE id=" . $mysqli->real_escape_string($this->id)
                          );

                //echo $query . "<br/>\n";

                $res = $mysqli->query($query);
                if ($mysqli->errno) {
                        $this->err = $mysqli->error;
                        return false;
                }

                $query = ("DELETE FROM tag_posts WHERE post_id=" . $this->id . ";\n");
                //echo $query . "<br/>\n";
                $mysqli->query($query);
                if ($mysqli->errno) {
                        $this->err = $mysqli->error;
                        return false;
                }

                $query = ("DELETE FROM channel_posts WHERE post_id=" . $this->id . ";\n");
                //echo $query . "<br/>\n";
                $mysqli->query($query);
                if ($mysqli->errno) {
                        $this->err = $mysqli->error;
                        return false;
                }

                $query = ("DELETE FROM post_docs WHERE post_id=" . $this->id . ";\n" );
                //echo $query . "<br/>\n";
                $mysqli->query($query);
                if ($mysqli->errno) {
                        $this->err = $mysqli->error;
                        return false;
                }

                $query = ("DELETE FROM account_posts WHERE post_id=" . $this->id . ";\n" );
                //echo $query . "<br/>\n";
                $mysqli->query($query);
                if ($mysqli->errno) {
                        $this->err = $mysqli->error;
                        return false;
                }

                $this->post_to_account($this->from);
                $this->store_links();

                return true;
        }

        public function parse($visitor) 
        {
                $html = htmlspecialchars($this->text);
                //echo "<pre>html 1 = $html</pre>";

                $html = $this->parse_links($html, $visitor);
                //echo "<pre>html 2 = $html</pre>";

                $html = $this->parse_paragraphs($html);
                //echo "<pre>html 3 = $html</pre>";

                $this->text_html = $html;
                return TRUE;
        }

        public function parse_paragraphs($text) 
        {
                $lines = preg_split('|\n|', $text);                
                $para = false;
                $html = "";
                for ($i = 0; $i < count($lines); $i++) {
                        $line = trim($lines[$i]);
                        if (strlen($line) == 0) {
                                if ($para) {
                                        $html .= "</p>\n";
                                        $para = false;
                                }
                        } else {
                                if (!$para) {
                                        $html .= "<p>\n";
                                        $para = true;
                                }
                        }
                        $html .= $lines[$i];
                }
                if ($para) {
                        $html .= "</p>\n";
                        $para = false;
                }
                return $html;
        }

        /*
          @username
          #tag
          #tag -> channel
          [http://...]
          [https://...]
        */
        public function parse_links($text, $visitor) 
        {
                global $base_url;

                /* Add a white space for convenience. FIXME: Is this acceptable? */
                $text = $text . " ";

                $len = strlen($text);
                $html = "";
                $state = 0;
                $buffer = "";
                $username = "";

                $tags = array();
                $channels = array();
                $accounts = array();

                for ($i = 0; $i < $len; $i++) {
                        $c = substr($text, $i, 1);

                        switch ($state) {
                        
                        case 0:
                                if ($c == "@") {
                                        $state = 4;
                                        $buffer = "";
                                } else if ($c == "#") {
                                        $state = 10;
                                        $buffer = "";
                                } else if ($c == "[") {
                                        $state = 11;
                                        $buffer = "";
                                } else {
                                        $html .= $c;
                                }
                                break;

                        case 4:
                                if (!valid_symbol_char($c)) {
                                        $state = 0;
                                        if (is_valid_username($buffer)) {
                                                $account = new Account();
                                                if ($account->load("username", $buffer)) {
                                                        $html .= "<a href=\"" . $base_url . "/people/" . $buffer . "/notebook/\">@" . $buffer . "</a>";
                                                        $accounts[] = $account->id;
                                                } else {
                                                        $html .= "@" . $buffer;
                                                }
                                        } else {
                                                $html .= "@" . $buffer;
                                        }
                                        $html .= $c;
                                } else { 
                                        $buffer .= $c;
                                }
                                break;
                
                        case 10:
                                if (!valid_symbol_char($c)) {

                                        $state = 0;
                                        $tag = Tag::load($buffer, $this->lang);
                                        if (!$tag) {
                                                $tag = new Tag(null, $buffer, $this->lang);
                                                if (!$tag->store()) {
                                                        return false;
                                                }
                                        }
                                        $tags[] = $tag;

                                        $channel_id = Channel::id_from_tag($buffer, 
                                                                           $this->lang);
                                        if ($channel_id) {
                                                $channels[] = $channel_id;
                                                $html .= "<a href=\"" . $base_url . "/people/" . $visitor->username . "/channel/$channel_id/" . $this->lang . "/\">#" . $buffer . "</a>";
                                        } else  {
                                                //$tags[] = new Tag($buffer, $this->lang);
                                                $html .= "<a href=\"" . $base_url . "/people/" . $visitor->username . "/tag/$buffer/" . $this->lang . "/\">#" . $buffer . "</a>";
                                        }
                                        $html .= $c;
                                } else { 
                                        $buffer .= $c;
                                }
                                break;

                        case 11:
                                if ($c == "]") {
                                        $state = 0;
                                        $url = $buffer;
                                        $parts = parse_url($url);
                                        $html .= "[<a href=\"$url\">" . $parts['host'] . "</a>]";
                                } else { 
                                        $buffer .= $c;
                                }
                                break;
                        }
                }

                $this->tags = $tags;
                $this->channels = $channels;
                $this->accounts = $accounts;

                return $html;
        }
}

?>
