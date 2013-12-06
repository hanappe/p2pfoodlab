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

class Thread
{
        public $id;
        public $posts;
        public $err;

        public function load($id) 
        {
                global $mysqli;

                $this->id = $id;
                $this->err = NULL;
                $this->posts = NULL;

                $query = ("SELECT p.`id`, p.`from`, p.`thread`, p.`replyto`, p.`date`, p.`text`, "
                          . "p.`text_html`, p.`lang` FROM posts p WHERE " 
                          . "(p.`thread`=" . $id . " OR p.`id`=" . $id . ")"
                          . " ORDER BY date ASC");

                //echo $query . "<br/>\n";

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

                //echo "Found " . count($this->posts) . " posts<br>";

                return true;
        }
}


?>
