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
require_once "post.model.php";

class Postbox
{
        public $posts;
        public $err;

        public function _load($query) 
        {
                global $mysqli;

                $this->err = NULL;
                $this->posts = NULL;

                //echo $query . "<br/>";

                $res = $mysqli->query($query);
                if (!$res) {
                        $this->err = "Failed to load the posts: " . $mysqli->error;
                        return false;
                }

                $posts = array();
        
                while ($row = $res->fetch_assoc()) {
                        $post = new Post();
                        $post->import($row);
                        $posts[] = $post;
                }
                
                $this->posts = $posts;

                return true;
        }

        public function add_post($post) 
        {
                if (!$this->posts) {
                        $this->posts = array();
                }
                $this->posts[] = $post;
        }

        public function load_account($account_id, $lang, $newest_first) 
        {
                global $mysqli;

                if ($lang) {
                        $query = ("SELECT p.`id`, p.`from`, p.`thread`, p.`replyto`, p.`date`, p.`text`, "
                                  . "p.`text_html`, p.`lang` FROM posts p, account_posts ap WHERE " 
                                  . "ap.`account_id`=" . $account_id . " "
                                  . "AND ap.`post_id`=p.`id` "
                                  . "AND p.`lang`='" . $mysqli->real_escape_string($lang->code) . "'");
                } else {
                        $query = ("SELECT p.`id`, p.`from`, p.`thread`, p.`replyto`, p.`date`, p.`text`, "
                                  . "p.`text_html`, p.`lang` FROM posts p, account_posts ap WHERE " 
                                  . "ap.`account_id`=" . $account_id . " "
                                  . "AND ap.`post_id`=p.`id`");
                }
                if ($newest_first)
                        $query .= "ORDER BY date DESC";
                else
                        $query .= "ORDER BY date DESC";

                return $this->_load($query);
        }

        public function load_persotag($host, $tag, $lang) 
        {
                global $mysqli;

                if ($lang) {
                        $query = ("SELECT p.`id` AS `id`, p.`from` AS `from`, p.`thread` AS `thread`, "
                                  . "p.`replyto` AS `replyto`, p.`date` AS `date`, p.`text` AS `text`, "
                                  . "p.`text_html` AS `text_html`, p.`lang` AS `lang` "
                                  . "FROM tags t, tag_posts tp, posts p "
                                  . "WHERE " 
                                  . "t.`text`='" . $mysqli->real_escape_string($tag) . "' "
                                  . "AND t.`lang`='" . $mysqli->real_escape_string($lang->code) . "' "
                                  . "AND tp.`tag_id`=t.`id` "
                                  . "AND tp.`post_id`=p.`id` "
                                  . "AND p.`from`= " . $host->id . " "
                                  . "ORDER BY date DESC");
                } else {
                        $query = ("SELECT p.`id` AS `id`, p.`from` AS `from`, p.`thread` AS `thread`, "
                                  . "p.`replyto` AS `replyto`, p.`date` AS `date`, p.`text` AS `text`, "
                                  . "p.`text_html` AS `text_html`, p.`lang` AS `lang` "
                                  . "FROM tags t, tag_posts tp, posts p "
                                  . "WHERE " 
                                  . "t.`text`='" . $mysqli->real_escape_string($tag) . "' "
                                  . "AND tp.`tag_id`=t.`id` "
                                  . "AND tp.`post_id`=p.`id` "
                                  . "AND p.`from`= " . $host->id . " "
                                  . "ORDER BY date DESC");
                }
                return $this->_load($query);
        }

        public function load_persochannel($host, $channel, $lang) 
        {
                global $mysqli;

                if ($lang) {
                        $query = ("SELECT p.`id` AS `id`, p.`from` AS `from`, p.`thread` AS `thread`, "
                                  . "p.`replyto` AS `reply`, p.`date` AS `date`, p.`text` AS `text`, "
                                  . "p.`text_html` AS `text_html`, p.`lang` AS `lang` "
                                  . "FROM posts p, channel_posts c "
                                  . "WHERE " 
                                  . "c.`channel_id`='" . $channel->id . "' "
                                  . "AND c.`lang`='" . $mysqli->real_escape_string($lang->code) . "' "
                                  . "AND c.`post_id`=p.id "
                                  . "AND p.`from`= " . $host->id . " "
                                  . "ORDER BY date DESC");
                } else {
                        $query = ("SELECT p.`id` AS `id`, p.`from` AS `from`, p.`thread` AS `thread`, "
                                  . "p.`replyto` AS `reply`, p.`date` AS `date`, p.`text` AS `text`, "
                                  . "p.`text_html` AS `text_html`, p.`lang` AS `lang` "
                                  . "FROM posts p, channel_posts c "
                                  . "WHERE " 
                                  . "c.`channel_id`='" . $channel->id . "' "
                                  . "AND c.`post_id`=p.id "
                                  . "AND p.`from`= " . $host->id . " "
                                  . "ORDER BY date DESC");
                }
                return $this->_load($query);
        }

        public function load_tag($tag, $lang) 
        {
                global $mysqli;

                if ($lang) {
                        $query = ("SELECT p.`id` AS `id`, p.`from` AS `from`, p.`thread` AS `thread`, "
                                  . "p.`replyto` AS `replyto`, p.`date` AS `date`, p.`text` AS `text`, "
                                  . "p.`text_html` AS `text_html`, p.`lang` AS `lang` "
                                  . "FROM tags t, tag_posts tp, posts p "
                                  . "WHERE " 
                                  . "t.`text`='" . $mysqli->real_escape_string($tag) . "' "
                                  . "AND t.`lang`='" . $mysqli->real_escape_string($lang->code) . "' "
                                  . "AND tp.`tag_id`=t.`id` "
                                  . "AND tp.`post_id`=p.`id` "
                                  . "ORDER BY date DESC");
                } else {
                        $query = ("SELECT p.`id` AS `id`, p.`from` AS `from`, p.`thread` AS `thread`, "
                                  . "p.`replyto` AS `replyto`, p.`date` AS `date`, p.`text` AS `text`, "
                                  . "p.`text_html` AS `text_html`, p.`lang` AS `lang` "
                                  . "FROM tags t, tag_posts tp, posts p "
                                  . "WHERE " 
                                  . "t.`text`='" . $mysqli->real_escape_string($tag) . "' "
                                  . "AND tp.`tag_id`=t.`id` "
                                  . "AND tp.`post_id`=p.`id` "
                                  . "ORDER BY date DESC");
                }
                return $this->_load($query);
        }

        public function load_channel($channel, $lang) 
        {
                global $mysqli;

                if ($lang) {
                        $query = ("SELECT p.`id` AS `id`, p.`from` AS `from`, p.`thread` AS `thread`, "
                                  . "p.`replyto` AS `reply`, p.`date` AS `date`, p.`text` AS `text`, "
                                  . "p.`text_html` AS `text_html`, p.`lang` AS `lang` "
                                  . "FROM posts p, channel_posts c "
                                  . "WHERE " 
                                  . "c.`channel_id`='" . $channel->id . "' "
                                  . "AND c.`lang`='" . $mysqli->real_escape_string($lang->code) . "' "
                                  . "AND c.`post_id`=p.id "
                                  . "ORDER BY date DESC");
                } else {
                        $query = ("SELECT p.`id` AS `id`, p.`from` AS `from`, p.`thread` AS `thread`, "
                                  . "p.`replyto` AS `reply`, p.`date` AS `date`, p.`text` AS `text`, "
                                  . "p.`text_html` AS `text_html`, p.`lang` AS `lang` "
                                  . "FROM posts p, channel_posts c "
                                  . "WHERE " 
                                  . "c.`channel_id`='" . $channel->id . "' "
                                  . "AND c.`post_id`=p.id "
                                  . "ORDER BY date DESC");
                }
                return $this->_load($query);
        }

        public function load_recent($lang) 
        {
                global $mysqli;

                if ($lang) {
                        $query = ("SELECT p.`id`, p.`from`, p.`thread`, p.`replyto`, p.`date`, p.`text`, "
                                  . "p.`text_html`, p.`lang` FROM posts p WHERE 1 " 
                                  . "AND p.`lang`='" . $mysqli->real_escape_string($lang->code) . "' "
                                  . "ORDER BY date DESC LIMIT 30");
                } else {
                        $query = ("SELECT p.`id`, p.`from`, p.`thread`, p.`replyto`, p.`date`, p.`text`, "
                                  . "p.`text_html`, p.`lang` FROM posts p WHERE 1 " 
                                  . "ORDER BY date DESC LIMIT 30");
                }

                return $this->_load($query);
        }

}


?>
