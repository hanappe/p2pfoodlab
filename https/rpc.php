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

require_once "error.inc.php";
require_once "db.inc.php";
require_once "input.inc.php";
require_once "account.model.php";
require_once "post.model.php";
require_once "locale.model.php";

session_start();

$visitor_account = load_visitor_account();
if ($visitor_account == NULL)
        unauthorized("No valid session");

db_connect();

$json = file_get_contents('php://input');
$request = json_decode($json);


if (!$request) {
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
        jsonBadRequest($msg);    
}

if ($request->op == "set_locale") {
        $valid = false;
        $lang = get_interface_languages();
        for ($i = 0; $i < count($lang); $i++) {
                if ($request->locale == $lang[$i]->code) {
                        $valid = true;
                        break;
                }
        }
        if (!$valid) jsonBadRequest("Unsupported language selection");    
        
        $r = $visitor_account->set_locale($request->locale);
        if ($r == false) jsonInternalServerError("Database error");

        jsonSuccess();

} else if ($request->op == "set_lang") {
        $valid = false;
        $lang = get_languages();
        for ($i = 0; $i < count($lang); $i++) {
                if ($request->lang == $lang[$i]->code) {
                        $valid = true;
                        break;
                }
        }
        if (!$valid) jsonBadRequest("Unsupported language selection");    
        
        $r = $visitor_account->set_lang($request->lang, $request->select);
        if ($r == false) jsonInternalServerError("Database error");

        jsonSuccess();

} else if ($request->op == "select_sensorbox") {
        
        if (!property_exists($request, "gid")
            || !is_valid_id($request->gid))
                jsonBadRequest("Wazzup?");
        if (!property_exists($request, "select")
            || (!($request->select === true)
                && !($request->select === false)))
                jsonBadRequest("Wazzzup?");

        $r = $visitor_account->set_sensorbox($request->gid, $request->select);
        if ($r == false) jsonInternalServerError("Database error");

        jsonSuccess();

} else if ($request->op == "select_datastream") {
        
        if (!property_exists($request, "gid")
            || !property_exists($request, "did")
            || !is_valid_id($request->gid)
            || !is_valid_id($request->did))
                jsonBadRequest("Wazzup?");
        if (!property_exists($request, "select")
            || (!($request->select === true)
                && !($request->select === false)))
                jsonBadRequest("Wazzzup?");

        $r = $visitor_account->set_datastream($request->gid,
                                              $request->did,
                                              $request->select);
        if ($r == false) jsonInternalServerError("Database error");

        jsonSuccess();

} else if ($request->op == "remove_doc") {
        
        if (!property_exists($request, "post_id")
            || !is_valid_id($request->post_id))
                jsonBadRequest("invalid post ID: " 
                               . $request->post_id);

        if (!property_exists($request, "doc_id")
            || !is_valid_id($request->doc_id))
                jsonBadRequest("invalid post ID: " 
                               . $request->doc_id);

        $post = new Post();

        if (!$post->load("id", $request->post_id))
                jsonBadRequest("could not find the requested post: " 
                               . $post->err);

        if ($post->from != $visitor_account->id)
                jsonUnauthorized("no comments...");
        
        if (!$post->remove_doc($request->doc_id))
                jsonInternalServerError("Failed to remove doc " 
                                        . $request->doc_id);
        
        jsonSuccess();

} else if ($request->op == "set_homepage") {

        if (!property_exists($request, "html"))
                jsonBadRequest("invalid set_homepage request");

        // FIXME: validate the HTML!!!

        $visitor_account->set_homepage($request->html);
        jsonSuccess();
 }

?>
