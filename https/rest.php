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

require_once "config.inc.php";
require_once "locale.model.php";
require_once "db.inc.php";
require_once "input.inc.php";
require_once "error.inc.php";
require_once "account.model.php";
require_once "post.model.php";
require_once "postbox.model.php";
require_once "doc.model.php";
require_once "thread.model.php";
require_once "threadlist.model.php";
require_once "tag.model.php";
require_once "channel.model.php";

session_start();
db_connect();

$target = input_get('target');
$id = input_get('id');
$format = input_get('format');

header('Content-type: application/json'); 

if ($target == "post") {

        if (!is_valid_id($id)) {
                jsonBadRequest("Invalid ID ($id)");
        }

        $post = new Post();
        if (!$post->load("id", $id)) 
                jsonBadRequest("Invalid post ($id)");
        if (!$post->load_docs()) 
                jsonInternalServerError("Failed to load the post");

        $response = new StdClass();
        $response->success = true;
        $response->id = $post->id;
        $response->from = $post->from;
        if ($post->thread_id != 0) 
                $response->thread = $post->thread_id;
        $response->text = $post->text;
        $response->date = $post->date;
        $response->lang = $post->lang;        
        if (count($post->docs) > 0) {
                $response->docs = array();
                for ($i = 0; $i < count($post->docs); $i++) {
                        $doc = new StdClass();
                        $doc->id = $post->docs[$i]->id;
                        $doc->path = $post->docs[$i]->path;
                        $response->docs[] = $doc;
                }
        }
        echo json_encode($response) . "\n";
        exit();
 }
?>
