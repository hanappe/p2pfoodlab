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

class Tag
{
        public $id;
        public $text;
        public $lang;

        function __construct($id, $text, $lang) {
                $this->id = $id;
                $this->text = $text;
                $this->lang = $lang;
        }
                
        public static function is_valid($s) {
                return preg_match('/^[a-zA-Z][a-zA-Z0-9]+$/', $s);
        }

        public function store() 
        {
                global $mysqli;
                
                if ($this->id) 
                        return false;

                $query = ("INSERT INTO tags VALUES ("
                          . "NULL,"
                          . "'" . $mysqli->real_escape_string($this->text) . "',"
                          . "'" . $mysqli->real_escape_string($this->lang) . "')");

                //echo $query . "<br/>\n";

                $res = $mysqli->query($query);
                if ($mysqli->errno) {
                        //echo $mysqli->error . "<br/>\n";
                        return false;
                }

                $this->id = $mysqli->insert_id;
                return true;
        }

        public static function load($text, $lang)
        {
                global $mysqli;

                $query = ("SELECT * FROM `tags` WHERE " 
                          . "`text`='" . $mysqli->real_escape_string($text) . "' "
                          . "AND `lang`='" . $mysqli->real_escape_string($lang) . "'");

                //echo $query . "<br/>\n";

                $res = $mysqli->query($query);
                if ($mysqli->errno) {
                        echo $mysqli->error . "<br/>\n";
                        return false;
                }

                if (!$res || ($res->num_rows != 1)) 
                        return false;

                $row = $res->fetch_assoc();
                
                $tag = new Tag($row['id'], $row['text'], $row['lang']);

                return $tag;
        }
}

?>
