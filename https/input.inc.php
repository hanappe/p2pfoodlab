<?php
/*  
    P2P Food Lab 

    P2P Food Lab is an open, collaborative platform to develop an
    alternative food value system,.

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

function input_get($s)
{
        return isset($_REQUEST[$s])? $_REQUEST[$s] : NULL;
}

function input_get_visitor()
{
        return trim(input_get('visitor'));
}

function input_get_host()
{
        return trim(input_get('host'));
}

function input_get_username()
{
        return trim(input_get('username'));
}

function is_valid_username($s)
{
        return preg_match('/^[a-zA-Z][a-zA-Z0-9]{2,23}$/', $s);
}

function is_valid_userid($s)
{
        return preg_match('/^[0-9]+$/', $s);
}

function is_valid_id($s)
{
        return preg_match('/^[0-9]+$/', $s);
}

function input_get_email()
{
        return trim(input_get('email'));
}

function is_valid_email($email)
{
        return filter_var($email, FILTER_VALIDATE_EMAIL);
}

function input_get_password()
{
        return input_get('pw');
}
function is_valid_password($s)
{
        $len = strlen($s);
        if ($len < 8) 
                return false;
        if ($len > 256) 
                return false;
        if (preg_match("/^[a-zA-Z]+$/", $s)) // not all characters
                return false;
        if (preg_match("/^[0-9]+$/", $s))  // not all numbers
                return false;
        $c = substr($s, 0, 1);
        $count = 1;
        for ($i = 1; $i < $len; $i++) 
                if (substr($s, $i, 1) == $c)
                        $count++;
        if ($count == $len)
                return false;

        return true;
}

function input_get_pwhash()
{
        return input_get('pwhash');
}

function is_valid_pwhash($s)
{
        return preg_match('/^[a-z0-9]{32,32}$/', $s);
}

function is_valid_key($s)
{
        return preg_match('/^[a-zA-Z0-9]{8,8}-[a-zA-Z0-9]{4,4}-[a-zA-Z0-9]{4,4}-[a-zA-Z0-9]{4,4}-[a-zA-Z0-9]{12,12}$/', $s);
}

function input_get_text()
{
        return trim(input_get('text'));
}

function input_get_locale()
{
        return trim(input_get('locale'));
}

function is_valid_locale($s)
{
        $lang = get_interface_languages();
        for ($i = 0; $i < count($lang); $i++) {
                if ($s == $lang[$i]->code)
                        return true;
        }
        return false;
}

function input_get_lang()
{
        $l = input_get('lang');

        if (($l == NULL) || (strlen($l) == 0))
                $l = Locale::$code;
        
        if (!preg_match('/^[a-z]{2,2}$/', $l)) {
                badRequest("Requested language not yet supported or not valid ('$l').");
        }

        $lang = new Language();
        if ($lang->load($l) == false)
                badRequest("Requested language not yet supported or not valid.");

        return $lang;
}

function input_get_replyto()
{
        $replyto = input_get('replyto');
        if ($replyto == NULL)
                return NULL;
        if (!preg_match("/^[0-9]+$/", $replyto))
                badRequest("Invalid reply ID");
        return $replyto;
}

function input_get_thread()
{
        $thread = input_get('thread');
        if ($thread == NULL)
                return NULL;
        if (!preg_match("/^[0-9]+$/", $thread))
                badRequest("Invalid thread ID");
        return $thread;
}

function input_get_url()
{
        global $base_url;

        $url = input_get('url');
        if (!filter_var($url, FILTER_VALIDATE_URL))
                badRequest("Missing or bad referer URL, url=$url");
        if (strpos($url, $base_url) === FALSE) 
                badRequest("Invalid referer URL, url=$url");
        return $url;
}

function input_get_gid()
{
        $gid = input_get('gid');

        if (($gid == NULL) || (strlen($gid) == 0))
                return null;

        if (!preg_match('/^[0-9]+$/', $gid)) {
                badRequest("Requested greenhouse index not valid ('$gid').");
        }

        return $gid;
}

function input_get_tag()
{
        $tag = input_get('tag');
        if (!Tag::is_valid($tag)) {
                badRequest("Invalid tag ('$tag').");
        }
        return $tag;
}

function input_get_channel_id()
{
        $id = input_get('channel');
        if (!preg_match('/^[0-9]+$/', $id)) 
                badRequest("Invalid channel ('$id').");
        return $id;
}

function input_get_post_id()
{
        $id = input_get('post');
        if (!preg_match('/^[0-9]+$/', $id)) 
                badRequest("Invalid post ('$id').");
        return $id;
}

?>
