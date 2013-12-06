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

class Channel
{
        public $id;
        public $lang;
        public $title;
        public $tags;

        public static function id_from_tag($tag, $lang)
        {
                global $mysqli;

                $query = ("SELECT `channel_id` FROM `channel_tags` ct, `tags` t WHERE " 
                          . "ct.`tag_id`=t.`id` "
                          . "AND t.`text`='" . $mysqli->real_escape_string($tag) . "' "
                          . "AND t.`lang`='" . $mysqli->real_escape_string($lang) . "'");

                echo $query . "<br/>\n";

                $res = $mysqli->query($query);
                if ($mysqli->errno) echo $mysqli->error . "<br/>\n";
                
                if (!$res || ($res->num_rows != 1)) return false;
                $row = $res->fetch_assoc();

                return $row['channel_id'];
        }

        public static function load_all($lang)
        {
                global $mysqli;

                if ($lang) {
                        $query = ("SELECT `id`, `lang`, `title` FROM channels "
                                  . "WHERE `lang`='" . $mysqli->real_escape_string($lang->code) . "' ORDER by `order`");
                } else {
                        $query = ("SELECT `id`, `lang`, `title` FROM channels "
                                  . "WHERE `lang`='en' ORDER by `order`");
                }
                //echo $query . "<br/>\n";

                $res = $mysqli->query($query);
                if ($mysqli->errno) {
                        echo $mysqli->error . "<br/>\n";
                        return false;
                }
                
                $channels = array();

                while ($row = $res->fetch_assoc()) {
                        $channel = new Channel();
                        $channel->import($row);
                        $channel->load_tags($lang);
                        $channels[] = $channel;
                }

                return $channels;
        }

        public function load($id, $lang) 
        {
                global $mysqli;

                if ($lang) {
                        $query = ("SELECT `id`, `lang`, `title` FROM channels WHERE "
                                  . "id=" . $id . " AND `lang`='" . $mysqli->real_escape_string($lang->code) . "'");
                } else {
                        $query = ("SELECT `id`, `lang`, `title` FROM channels WHERE "
                                  . "id=" . $id . " AND `lang`='en'");
                }
                //echo $query . "<br/>\n";

                $res = $mysqli->query($query);
                if (!$res || ($res->num_rows != 1)) return false;
                $row = $res->fetch_assoc();
                $this->import($row);

                return true;
        }

        public function load_tags($lang) 
        {
                global $mysqli;
                
                $this->tags = array();
                
                $query = ("SELECT t.text as text FROM `channel_tags` ct, `tags` t WHERE " 
                          . "ct.`channel_id`=" . $this->id . " "
                          . "AND ct.`tag_id`=t.`id` "
                          . "AND t.`lang`='" . $mysqli->real_escape_string($this->lang) . "'");
                
                //echo $query . "<br/>\n";
                
                $res = $mysqli->query($query);
                if ($mysqli->errno) {
                        echo $mysqli->error . "<br/>\n";
                        return false;
                }
                
                while ($row = $res->fetch_assoc()) {
                        $this->tags[] = $row['text'];
                }

                return true;
        }

        public function import($row) 
        {
                $this->id = $row['id'];
                $this->lang = $row['lang'];
                $this->title = $row['title'];
        }
}

?>
