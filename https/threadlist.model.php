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
require_once "thread.model.php";

class ThreadList
{
        public $threads;
        public $err;

        public function convert($postbox) 
        {
                $this->threads = array();

                $thread_ids = array();

                for ($i = 0; $i < count($postbox->posts); $i++) {
                        
                        $post = $postbox->posts[$i];
                        
                        $thread_id = $post->thread_id;
                        if ($thread_id == 0)
                                $thread_id = $post->id;

                        if (!isset($thread_ids[$thread_id])) {
                                $thread = new Thread();
                                if (!$thread->load($thread_id))
                                        internalServerError("Failed to load thread " . $thread_id);
                                $this->threads[] = $thread;
                                $thread_ids[$thread_id] = $thread;
                        }
                }

                //echo "Found " . count($this->threads) . " threads<br>";

        }
}

?>
