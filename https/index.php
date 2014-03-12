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
require_once "page.model.php";
require_once "securimage/securimage.php";
require_once "opensensordata.inc.php";

session_start();
db_connect();


$visitor_account = load_visitor_account();
$host_account = load_host_account();

Locale::init($visitor_account);

$op = input_get('op');
$view = input_get('view');
$lang = input_get_lang();
$page = NULL;

header('Content-type: text/html; charset=utf-8'); 

if ($view == "post") {

        $id = input_get_post_id();

        $post = new Post();
        if (!$post->load("id", $id)) 
                badRequest("Invalid post ($id)");

        $page = new PostPage($post, $lang, $visitor_account);
        $page->generate();

 } else if ($view == "channel") {
        $id = input_get_channel_id();
        $channel = new Channel();
        if (!$channel->load($id, $lang)) 
                badRequest("Invalid channel or locale");

        $page = new ChannelPage($channel, $lang);
        $page->generate();


 } else if ($view == "tag") {
        $tag = input_get_tag();
        $page = new TagPage($tag, $lang, $visitor_account);
        $page->generate();


 } else if ($view == "persotag") {
        $tag = input_get_tag();
        $page = new PersoTagPage($host_account, $tag, $lang, $visitor_account);
        $page->generate();


 } else if ($view == "persochannel") {
        $id = input_get_channel_id();
        $channel = new Channel();
        if (!$channel->load($id, $lang)) 
                badRequest("Invalid channel or locale");
        $page = new PersoChannelPage($host_account, $channel, $lang, $visitor_account);
        $page->generate();


 } else if ($view == "notebook") {
        $page = new Notebook($host_account,
                             $visitor_account,
                             $lang);
        $page->generate();


 } else if ($view == "account") {
        $page = new AccountPage($visitor_account);
        $page->generate();


 } else if ($view == "greenhouse") {
        $gid = input_get_gid();
        $page = new GreenhousePage($host_account,
                                   $visitor_account,
                                   $gid);
        $page->generate();


 } else if ($view == "login") {
        if ($op == "login") {
                $account = do_login();
                header("Location: ". $base_url . "/people/" . $account->username);
                db_close();
                exit(0);
        }
        $page = new LoginPage("");
        $page->generate();


 } else if ($view == "logout") {
        unset($_SESSION['account']);
        unset($_SESSION['timeout']);
        unset($_SESSION["locale"]);
        header("Location: " . $base_url);
        db_close();
        exit(0);


 } else if ($view == "createaccount") {
        if ($op == "create") {
                $account = create_account();
                $page = new EmailSentPage($account);
                $page->generate();

        } else if ($op == "validate") {
                $visitor_account = validate_account();
                //echo "ok";
                header("Location: ". $base_url . "/people/" . $visitor_account->username);
                db_close();
                exit(0);
        }
        $page = new CreateAccountForm("", "", "", "", "");
        $page->generate();
        

 } else if ($view == "help") {
        $page = new HelpPage($visitor_account, $lang);
        $page->generate();

 } else if ($view == "conditions") {
        $page = new TermsAndConditions($visitor_account, $lang);
        $page->generate();

 } else if ($view == "posts") {
        $page = new RecentPostsPage($visitor_account, $lang);
        $page->generate();

 } else if ($view == "people") {
        $page = new PeoplePage($visitor_account, $lang);
        $page->generate();

 } else {
        $page = new IndexPage($visitor_account, $lang);
        $page->generate();
 }

?>
