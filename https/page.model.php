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

class Page
{
        public $visitor;
        public $show_menu;
        public $show_account_submenu;
        public $message;

        function __construct($visitor) 
        {
                $this->visitor = $visitor;
                $this->show_menu = true;
                $this->show_account_submenu = true;
                $this->message = false;
        }

        public function insert_title()
        {
        }

        public function insert_body() 
        {
        }

        public function generate() 
        {
                global $page;
                require_once "page.view.php";
                db_close();
                exit(0);
        }

}

class LoginPage extends Page
{
        public $username;

        function __construct($username) 
        {
                parent::__construct(NULL);
                $this->username = $username;
                $this->show_account_submenu = true;
        }

        public function insert_title()
        {
                echo "<span class='title'>" . _s("login") . "</span>\n";
        }

        public function insert_body() 
        {
                global $page;
                include "login.view.php";                 
        }
}

class CreateAccountForm extends Page
{
        public $username;
        public $email;
        public $pw;
        public $agree;
        public $locale;

        function __construct($username, $email, $pw, $agree, $locale) 
        {
                parent::__construct(NULL);
                $this->username = $username;
                $this->email = $email;
                $this->pw = $pw;
                $this->agree = $agree;
                $this->locale = $locale;
                $this->show_account_submenu = true;
        }

        public function insert_title()
        {
                echo "<span class='title'>" . _s("register") . "</span>\n";
        }

        public function insert_body() 
        {
                global $page;
                include "createaccount.form.view.php"; 
        }
}

class EmailSentPage extends Page
{
        public $account;

        function __construct($account) 
        {
                parent::__construct(NULL);
                $this->account = $account;
                $this->show_account_submenu = true;
        }

        public function insert_title()
        {
                echo "<span class='title'>" . _s("register") . "</span>\n";
        }

        public function insert_body() 
        {
                global $page, $account;
                include "createaccount.mail.view.php"; 
        }
}

class ThreadListViewer extends Page
{
        public $threadlist;
        public $accounts;
        
        function __construct($visitor) 
        {
                parent::__construct($visitor);
                $this->accounts = array();
        }

        public function get_account($id)
        {
                if (isset($this->accounts[$id])) {
                        $account = $this->accounts[$id];
                } else {
                        $account = new Account();
                        if (!$account->load("id", $id)) 
                                internalServerError("Can't find account " . $post->from);
                        $this->accounts[$id] = $account;
                }
                return $account;
        }

        public function insert_post($post, $first_post, $type, $expanded, $thread_size)
        {
                global $base_url;

                $post->load_docs();
                $account = $this->get_account($post->from);
                $from = $account->username;
                $url = url_s('people/' . $account->username);

                if ($first_post) 
                        echo "    <div class=\"note\" id=\"post_" . $post->id . "\">\n";
                else
                        echo "    <div class=\"reply\" id=\"post_" . $post->id . "\">\n";

                if ($first_post) 
                        echo "      <div class=\"note_type type_$type\">" . _s($type) . "</div>\n";

                echo "      <div class=\"note_text\">\n      " . $post->text_html . "\n    </div>\n";

                if (count($post->docs) > 0) {

                        if (0) {
                        echo "    <div class=\"note_docs\" id=\"note_docs_" . $post->id . "\">\n";
                        echo "      <div class=\"note_icons\" id=\"note_docs_" . $post->id . "_icons\">\n";
                        for ($j = 0; $j < count($post->docs); $j++) {
                                $src = $base_url . "/docs/" . $post->docs[$j]->path . "-medium.jpg";
                                echo "        <img src=\"$src\" class=\"note_photo\" />\n";
                        }
                        echo "      </div>\n";
                        echo "    </div>\n";



                        } else {
                        echo "    <div class=\"note_docs\" id=\"note_docs_" . $post->id . "\">\n";
                        echo "      <div class=\"note_icons\" id=\"note_docs_" . $post->id . "_icons\">\n";
                        $list = "[ ";
                        for ($j = 0; $j < count($post->docs); $j++) {
                                $src = $base_url . "/docs/" . $post->docs[$j]->path . "-thumb.jpg";
                                echo "      <a href=\"javascript:void(0);\" onclick=\"show_photos('note_docs_" . $post->id ."', photos_" . $post->id . ", $j, photos_" . $post->id ."_handler);\"><img src=\"$src\" class=\"doc_thumb\" /></a>\n";

                                $src = $base_url . "/docs/" . $post->docs[$j]->path . "-medium.jpg";
                                if ($j != 0) 
                                        $list .= ", '" . $src . "'";
                                else 
                                        $list .= "'" . $src . "'";
                        }
                        $list .= " ]";
                        echo "      </div>\n";
                        echo "      <script>\n";
                        echo "        var photos_" . $post->id . " = $list;\n";
                        echo "        var photos_" . $post->id . "_handler = function(s) { hide_photos('note_docs_" . $post->id ."'); };\n";
                        echo "      </script>\n";                        
                        echo "    </div>\n";
                        }
                }

                echo "    <div class=\"note_infopanel\">\n";
                echo "      <a class=\"note_author\" href=\"$url\">$from</a> \n";
                echo "       <span class=\"note_date\">" . $post->date . "</span>\n";

                if ($first_post && ($thread_size > 0)) {
                        $hide = _s("hide thread") . " ($thread_size)";
                        $show = _s("show thread") . " ($thread_size)";
                        $label = ($expanded)? $hide : $show;
                        $id = "thread_" . $post->id;
                        echo ("      <a href=\"javascript:void(0);\" class=\"note_op\" "
                              . "id=\"showhide_$id\" "
                              . "onclick=\"show_hide_thread('$id', '$show', '$hide')\">$label</a>\n");
                }
                if ($this->visitor != NULL) {
                        $thread = ($first_post)? $post->id : $post->thread_id;
                        echo ("      <a href=\"javascript:void(0);\" class=\"note_op\" "
                              . "onclick=\"do_reply('$thread',"
                              . "'" . $post->id . "'," 
                              . "'" . $post->lang . "')\">" . _s("reply") ."</a>\n");
                } else {
                        $url = $base_url . "/login";
                        echo "          <a href='$url' class=\"note_op\">" . _s("log in to reply"). "</a>\n";
                }
                if (($this->visitor != NULL) && ($this->visitor->id == $post->from)) {
                        echo ("      <a href=\"javascript:void(0);\" class=\"note_op\" "
                              . "onclick=\"do_edit('" . $post->id . "')\">" . _s("edit") ."</a>\n");
                }
                $url = $base_url . "/post/" . $post->id;
                echo "          <a href='$url'  class='note_op'>" . _s("permalink"). "</a>\n";
                echo "        </div>\n";
                echo "      </div>\n\n";
        }

        public function insert_thread($thread, $expand)
        {
                $post = $thread->posts[0];
                $post->load_channels();

                if ($post->in_channel(1)) {
                        $type = "question";
                } else if ($post->in_channel(2)) {
                        $type = "observation";
                } else if ($post->in_channel(3)) {
                        $type = "advice";
                } else if ($post->in_channel(4)) {
                        $type = "funny";
                } else {
                        $type = "note";
                }
                $color_scheme = "post_" . $type;


                $post->load_docs();
                if ((strlen($post->text) == 0) && (count($post->docs) == 0)) {
                        return;
                }

                echo "  <div class=\"strip\">\n";
                echo "    <div class=\"content_box $color_scheme\">\n";

                $thread_size = count($thread->posts) - 1;

                $this->insert_post($post, true, $type, $expand, $thread_size);

                if ($thread_size > 0) {
                        $visibility = $expand? "visible" : "hidden";
                        echo "    <div id=\"thread_" . $thread->id . "\" class=\"$visibility\">\n";
                        for ($i = 1; $i < count($thread->posts); $i++) {
                                $post = $thread->posts[$i];
                                $this->insert_post($post, false, "reply", false, false);
                        }                
                        echo "      </div>\n";
                }

                echo "    </div>\n";
                echo "  </div>\n\n";
        }

        public function insert_threads($expand)
        {
                for ($i = 0; $i < count($this->threadlist->threads); $i++) {
                        $thread = $this->threadlist->threads[$i];
                        $this->insert_thread($thread, $expand);
                }
        }
}

class PostPage extends ThreadListViewer
{
        public $post;

        function __construct($post, $lang, $visitor) 
        {
                parent::__construct($visitor);
                $this->post = $post;
                $this->lang = $lang;
        }

        public function insert_title()
        {
                echo "<span class='title'>" . _s("post") . "</span>\n";
        }

        public function insert_body() 
        {
                $postbox = new Postbox();
                $postbox->add_post($this->post);
                $this->threadlist = new ThreadList();
                $this->threadlist->convert($postbox);
                $this->insert_threads(true);
        }
}

class TagPage extends ThreadListViewer
{
        public $tag;
        public $lang;

        function __construct($tag, $lang, $visitor) 
        {
                parent::__construct($visitor);
                $this->tag = $tag;
                $this->lang = $lang;
        }

        public function insert_title()
        {
                echo "<span class='title'>" . $this->tag . "</span>\n";
        }

        public function insert_body() 
        {
                $postbox = new Postbox();
                if (!$postbox->load_tag($this->tag, $this->lang)) {
                        internalServerError($postbox->err);
                }
                $this->threadlist = new ThreadList();
                $this->threadlist->convert($postbox);
                $this->insert_threads(false);
        }
}

class ChannelPage extends ThreadListViewer
{
        public $channel;
        public $lang;

        function __construct($channel, $lang, $visitor) 
        {
                parent::__construct($visitor);
                $this->channel = $channel;
                $this->lang = $lang;
        }

        public function insert_title()
        {
                echo "<span class='title'>" . $this->channel->title . "</span>\n";
        }

        public function insert_body() 
        {
                $postbox = new Postbox();
                if (!$postbox->load_channel($this->channel, $this->lang)) {
                        internalServerError($postbox->err);
                }
                $this->threadlist = new ThreadList();
                $this->threadlist->convert($postbox);
                $this->insert_threads(false);
        }
}

class PersonalPage extends ThreadListViewer
{
        public $host;
        public $lang;
        public $group;
        public $select;

        function __construct($host, $visitor, $lang, $select) 
        {
                parent::__construct($visitor);
                $this->host = $host;
                $this->lang = $lang;
                $this->select = $select;

                if (!$host->load_sensorboxes()) {
                        internalServerError("Database error");
                }
        }

        public function insert_title()
        {
                global $base_url;
                $href = url_s('people/' . $this->host->username);
                echo "<a class=\"title\" href=\"$href\">" . $this->host->username . "</a>\n";
        }

        public function insert_notebook_menu()
        {
                $base = url_s('people/' . $this->host->username . '/notebook/');
                $np = $this->host->count_messages();
                $n = count($this->host->lang);

                if (($np <= 0) 
                    && (($this->visitor == null) 
                        || ($this->host->id != $this->visitor->id))) 
                        return;

                $u = "";
                if ($this->select == "notebook")
                        $u = "selected_menu";

                echo "              <div class=\"menu\">\n";
                echo "                <a href=\"$base\" class=\"$u\">" . _s("notebook") . "</a>\n";
                echo "              </div>\n";
                echo "              <div class=\"submenu\">\n";

                if ($n > 1) {
                        for ($i = 0; $i < $n; $i++) {
                                $url = $base . $this->host->lang[$i];
                                $l = $this->host->lang[$i];
                                $u = "";
                                if (($this->lang != NULL) 
                                    && ($this->lang->code == $this->host->lang[$i])) {
                                        $u = "selected_menu";
                        }
                                echo ("        <a href=\"$url\" class=\"submenu $u\">$l</a>\n");
                        }
                }
                echo "              </div>";
        }

        public function insert_dataset_menu()
        {
                global $mysqli;

                $base = url_s('people/' . $this->host->username . '/dataset/');
                $n = count($this->host->sensorbox);

                if (($n <= 0) 
                    && (($this->visitor == null) 
                        || ($this->host->id != $this->visitor->id))) 
                        return;

                $u = "";
                if ($this->select == "dataset")
                        $u = "selected_menu";

                echo "              <div class=\"menu\">\n";
                echo "                <a href=\"$base\" class=\"$u\">" . _s("datasets") . "</a>\n";
                echo "              </div>\n";
                echo "              <div class=\"submenu\">\n";

                if ($n > 1) {
                        for ($i = 0; $i < $n; $i++) {
                                $url = $base . $this->host->sensorbox[$i];
                                $u = "";
                                $num = $i + 1;
                                if (isset($this->group) 
                                    && ($this->group->id == $this->host->sensorbox[$i]))
                                        $u = "selected_menu";
                        
                                echo ("        <a href=\"$url\" class=\"submenu $u\">$num</a>\n");
                        }
                }

                echo "              </div>";
        }
}

class PersoTagPage extends PersonalPage
{
        public $tag;

        function __construct($host, $tag, $lang, $visitor) 
        {
                parent::__construct($host, $visitor, $lang, "nothing");
                $this->tag = $tag;
        }

        public function insert_taglink() 
        {
                global $base_url;

                $fmt = _s("View all %s notes.");
                $anchor = sprintf($fmt, "#" . $this->tag);
                $url = $base_url . "/tag/" . $this->tag . "/" . $this->lang->code;
                echo "    <div class=\"strip\">\n";
                echo "        <div class=\"content_box frame\">\n";
                echo "            <div class=\"margin\">\n";
                echo "                <a href=\"$url\">$anchor</a>\n";
                echo "            </div>\n";
                echo "        </div>\n";
                echo "    </div>\n";
        }

        public function insert_notebook_menu()
        {
                $base = url_s('people/' . $this->host->username . '/notebook/');
                $n = count($this->host->lang);
                $u = "";
                if ($this->select == "notebook")
                        $u = "selected_menu";

                echo "              <div class=\"menu\">\n";
                echo "                <a href=\"$base\" class=\"$u\">" . _s("notebook") . "</a>\n";
                echo "              </div>\n";
                echo "              <div class=\"submenu\">\n";
                echo "              <span class=\"standout\">" . $this->tag . "</span> - ";
                if ($n > 1) {
                        for ($i = 0; $i < $n; $i++) {
                                $url = $base . $this->host->lang[$i];
                                $l = $this->host->lang[$i];
                                $u = "";
                                if (($this->lang != NULL) 
                                    && ($this->lang->code == $this->host->lang[$i])) {
                                        $u = "selected_menu";
                        }
                                echo ("        <a href=\"$url\" class=\"submenu $u\">$l</a>\n");
                        }
                }
                echo "              </div>";
        }

        public function insert_body() 
        {
                $this->insert_taglink();                
                $postbox = new Postbox();
                if (!$postbox->load_persotag($this->host, $this->tag, $this->lang)) {
                        internalServerError($postbox->err);
                }
                $this->threadlist = new ThreadList();
                $this->threadlist->convert($postbox);
                $this->insert_threads(false);
        }
}

class PersoChannelPage extends PersonalPage
{
        public $channel;

        function __construct($host, $channel, $lang, $visitor) 
        {
                parent::__construct($host, $visitor, $lang, "nothing");
                $this->channel = $channel;
        }

        public function insert_channellink() 
        {
                global $base_url;

                $anchor = _s("View all channel notes.");
                $url = $base_url . "/channel/" . $this->channel->id . "/" . $this->lang->code;
                echo "    <div class=\"strip\">\n";
                echo "        <div class=\"content_box frame\">\n";
                echo "            <div class=\"margin\">\n";
                echo "                <a href=\"$url\">$anchor</a>\n";
                echo "            </div>\n";
                echo "        </div>\n";
                echo "    </div>\n";
        }
        public function insert_notebook_menu()
        {
                $base = url_s('people/' . $this->host->username . '/notebook/');
                $n = count($this->host->lang);
                $u = "";
                if ($this->select == "notebook")
                        $u = "selected_menu";

                echo "              <div class=\"menu\">\n";
                echo "                <a href=\"$base\" class=\"$u\">" . _s("notebook") . "</a>\n";
                echo "              </div>\n";
                echo "              <div class=\"submenu\">\n";
                echo "              <span class=\"standout\">" . $this->channel->title . "</span> - ";
                if ($n > 1) {
                        for ($i = 0; $i < $n; $i++) {
                                $url = $base . $this->host->lang[$i];
                                $l = $this->host->lang[$i];
                                $u = "";
                                if (($this->lang != NULL) 
                                    && ($this->lang->code == $this->host->lang[$i])) {
                                        $u = "selected_menu";
                        }
                                echo ("        <a href=\"$url\" class=\"submenu $u\">$l</a>\n");
                        }
                }
                echo "              </div>";
        }



        public function insert_body() 
        {
                $this->insert_channellink();                
                $postbox = new Postbox();
                if (!$postbox->load_persochannel($this->host, $this->channel, $this->lang)) {
                        internalServerError($postbox->err);
                }
                $this->threadlist = new ThreadList();
                $this->threadlist->convert($postbox);
                $this->insert_threads(false);
        }
}

class Notebook extends PersonalPage
{
        function __construct($host, $visitor, $lang) 
        {
                parent::__construct($host, $visitor, $lang, "notebook");
        }

        public function insert_editor() 
        {
                $lang_sel = $this->lang? $this->lang->code : $this->visitor->lang[0];
                $lang_opt = "['" . join("','", $this->visitor->lang) . "']"; 
                echo "    <div class=\"strip\">\n";
                echo "        <div class=\"content_box frame margin\">\n";
                echo "             <a href=\"javascript:void(0);\" onclick=\"do_newnote('$lang_sel', $lang_opt)\">" . _s("Write a new note.") . "</a>\n";
                echo "             <div id=\"newnote\" class=\"visible\"></div>\n";
                echo "        </div>\n";
                echo "    </div>\n";
        }

        public function insert_message() 
        {
                echo "    <div class=\"strip\">\n";
                echo "        <div class=\"content_box frame margin\">\n";
                echo "             " . _s("Start keeping notes and post your questions and observation using the link below. For more info, check the help section") . " (<a href=\"" . url_s('help#editing') . "\">" . _s("here") . "</a>).\n";
                echo "        </div>\n";
                echo "    </div>\n";
        }

        public function insert_body() 
        {
                if (($this->visitor != NULL)
                    && ($this->host != NULL)
                    && ($this->visitor->id == $this->host->id)
                    && ($this->host->count_messages() <= 0)) {
                        $this->insert_message();
                }
                if (($this->visitor != NULL)
                    && ($this->host != NULL)
                    && ($this->visitor->id == $this->host->id)) {
                        $this->insert_editor();
                }

                $postbox = new Postbox();
                if (!$postbox->load_account($this->host->id, $this->lang, true)) {
                        internalServerError($postbox->err);
                }
                $this->threadlist = new ThreadList();
                $this->threadlist->convert($postbox);
                $this->insert_threads(true);
        }
}

class IndexPage extends Page
{
        public $accounts;
        public $channels;

        function __construct($visitor, $lang) 
        {
                parent::__construct($visitor);

                $this->show_menu = false;

                $this->accounts = Account::load_all();
                if ($this->accounts === false)
                        internalServerError("Database error (Account::load_all)");

                $this->channels = Channel::load_all($lang);
                if ($this->channels === false)
                        internalServerError("Database error (Channels::load_all)");
        }

        public function insert_title()
        {
                echo "<span class='title'>" . _s("index") . "</span>\n";
        }

        public function insert_body() 
        {
                global $base_url;
                include "index.view.php";
        }
}

class PeoplePage extends Page
{
        public $accounts;
        public $channels;

        function __construct($visitor, $lang) 
        {
                parent::__construct($visitor);

                $this->accounts = Account::load_all();
                if ($this->accounts === false)
                        internalServerError("Database error (Account::load_all)");

                for ($i = 0; $i < count($this->accounts); $i++) {
                        $account = $this->accounts[$i];
                        if (!$account->load_sensorboxes()) {
                                internalServerError("Database error (Account::load_sensorboxes)");
                        }
                }
        }

        public function insert_title()
        {
                echo "<span class='title'>" . _s("participants") . "</span>\n";
        }

        public function insert_body() 
        {
                global $base_url;
                include "people.view.php";
        }
}

class AccountPage extends PersonalPage
{
        public $osd_account;
        public $datastreams;
        public $visible;

        function __construct($visitor) 
        {
                global $osd_server, $mysqli;

                parent::__construct($visitor, $visitor, "", "account");
                $this->show_account_submenu = true;

                $osd = new OpenSensorData($osd_server, null);
                $this->osd_account = $osd->get_account($visitor->osd_username);

                if (!$visitor->load_sensorboxes()) {
                        internalServerError("Database error");
                }

                $this->datastreams = array();
                $this->visible = array();
                
                for ($i = 0; $i < count($visitor->sensorbox); $i++) {
                        // FIXME: should be done elsewhere
                        $gid = $visitor->sensorbox[$i];
                        $query = "SELECT datastream_id AS id FROM sensorbox_datastreams WHERE group_id=" . $gid;
                        $this->visible[$gid] = true;
                        $res = $mysqli->query($query);
                        if (!$mysqli->errno) {
                                $this->datastreams[$gid] = array();
                                while ($row = $res->fetch_assoc()) {
                                        $id = $row['id'];
                                        $this->datastreams[$gid][$id] = true;
                                }
                        }
                }
        }

        public function insert_title()
        {
                echo "<span class='title'>" . _s("account") . "</span>\n";
        }

        public function insert_body() 
        {
                include "account.view.php"; 
        }
}

class MediaPage extends PersonalPage
{
        public $docs;
        public $visible;

        function __construct($host, $visitor) 
        {
                global $osd_server, $mysqli;

                parent::__construct($host, $visitor, "", "media");
                $this->show_account_submenu = true;
                $this->docs = Doc::loadall($host->id);
        }

        public function insert_title()
        {
                echo "<span class='title'>" . _s("media") . "</span>\n";
        }

        public function insert_upload() 
        {
                $lang_sel = $this->lang? $this->lang->code : $this->visitor->lang[0];
                $lang_opt = "['" . join("','", $this->visitor->lang) . "']"; 
                echo "    <div class=\"strip\">\n";
                echo "        <div class=\"content_box frame margin\">\n";
                echo "             <a href=\"javascript:void(0);\" onclick=\"do_upload()\">" . _s("Upload files.") . "</a>\n";
                echo "             <div id=\"newnote\" class=\"visible\"></div>\n";
                echo "        </div>\n";
                echo "    </div>\n";
        }

        public function insert_body() 
        {
                if (($this->visitor != NULL)
                    && ($this->host != NULL)
                    && ($this->visitor->id == $this->host->id)) {
                        $this->insert_upload();
                }

                $len = count($this->docs);
                for ($i = 0; $i < $len; $i++) {
                        $doc = $this->docs[$i];
                        if ($doc->type == "image/jpeg") {
                                $src = $base_url . "/docs/" . $doc->path . "-medium.jpg";
                                echo "      <div class=\"strip\">\n";
                                echo "        <div class=\"content_box\">\n";
                                echo "          <img src=\"$src\" />";
                                echo "        </div>";
                                echo "      </div>\n";
                        }
                }
        }
}

class HomePage extends PersonalPage
{
        public $host;
        public $index;
        public $select;
        public $errmsg;
        public $datastreams;

        function __construct($host, $visitor) 
        {
                global $osd_server, $mysqli;

                parent::__construct($host, $visitor, "", "homepage");

                $this->show_account_submenu = true;
        }

        public function insert_body() 
        {
                global $mysqli;
                $query = "SELECT html FROM homepage WHERE id=" . $this->host->id;
                $res = $mysqli->query($query);
                if (!$mysqli->errno) {
                        $row = $res->fetch_assoc();
                        echo "      <div class=\"strip\">\n";
                        echo "        <div class=\"content_box\">\n";
                        echo $row['html'];
                        echo "        </div>";
                        echo "      </div>\n";
                }
        }
}

class DatasetPage extends PersonalPage
{
        public $host;
        public $index;
        public $select;
        public $errmsg;
        public $datastreams;

        function __construct($host, $visitor, $gid) 
        {
                global $osd_server, $mysqli;

                parent::__construct($host, $visitor, "", "dataset");

                $this->host = $host;
                $this->show_account_submenu = true;

                $this->index = -1;
                $this->errmsg = NULL;

                for ($i = 0; $i < count($host->sensorbox); $i++) {
                        if ($host->sensorbox[$i] == $gid) {
                                $this->index = $i;
                                break;
                        }
                }
                if (($this->index == -1) && (count($host->sensorbox) > 0)) {
                        $this->index = 0;
                }
                if ($this->index >= 0) {
                        $osd = new OpenSensorData($osd_server, null, null);
                        $this->group = $osd->get_group($host->sensorbox[$this->index]);
                        if ($this->group === FALSE) {
                                $this->errmsg = "Connection to OpenSensorData.net failed";
                        }
                }

                if (isset($this->group)) {
                        // FIXME: should be done elsewhere
                        $query = "SELECT datastream_id AS id FROM sensorbox_datastreams WHERE group_id=" . $this->group->id;
                        $res = $mysqli->query($query);
                        if (!$mysqli->errno) {
                                $this->datastreams = array();
                                while ($row = $res->fetch_assoc()) {
                                        $id = $row['id'];
                                        $this->datastreams[$id] = true;
                                }
                        }
                }
        }

        public function insert_body() 
        {
                global $base_url;
                include "dataset.view.php"; 
        }
}

class HelpPage extends Page
{
        public $channels;

        function __construct($visitor, $lang) 
        {
                parent::__construct($visitor);

                //var_dump($lang);
                //echo "VISITOR"; var_dump($visitor);

                $this->channels = Channel::load_all($lang);
                if ($this->channels === false)
                        internalServerError("Database error (Channels::load_all)");
        }

        public function insert_title()
        {
                echo "<span class='title'>" . _s("help") . "</span>\n";
        }

        public function insert_body() 
        {
                if ($this->lang && $this->lang->code == 'fr')
                        include "help.view.fr.php"; 
                else
                        include "help.view.en.php"; 
        }
}

class TermsAndConditions extends Page
{
        public $lang;

        function __construct($visitor, $lang) 
        {
                parent::__construct($visitor);
                $this->lang = $lang;
        }

        public function insert_title()
        {
                echo "<span class='title'>" . _s("terms & conditions") . "</span>\n";
        }

        public function insert_body() 
        {
                if ($this->lang && $this->lang->code == 'fr')
                        include "conditions.view.fr.php"; 
                else
                        include "conditions.view.en.php"; 
        }
}

class ErrorPage extends Page
{
        public $msg;

        function __construct($visitor, $msg) 
        {
                parent::__construct($visitor);
                $this->msg = $msg;
        }

        public function insert_title()
        {
                echo "<span class='title'>" . _s("ERROR") . "</span>\n";
        }

        public function insert_body() 
        {
                include "error.view.php"; 
        }
}

class RecentPostsPage extends ThreadListViewer
{
        function __construct($visitor, $lang) 
        {
                parent::__construct($visitor);
                $this->lang = $lang;
        }

        public function insert_title()
        {
                echo "<span class='title'>" . _s("recent posts") . "</span>\n";
        }

        public function insert_body() 
        {
                $postbox = new Postbox();
                $postbox->load_recent($this->lang, 0, 100); 
                $this->threadlist = new ThreadList();
                $this->threadlist->convert($postbox);
                $this->insert_threads(false);
        }
}


?>
