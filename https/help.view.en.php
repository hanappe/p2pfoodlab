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
?>

<div class="strip">
  <div class="content_box frame margin">
    <h1>Index</h1>
    <ul>
      <li><a href="#introduction">Introduction</a><br/></li>
      <li><a href="#editing">Notebook</a><br/></li>
      <li><a href="#settings">Account settings</a></li>
      <li><a href="#settings">OpenSensorData.net</a></li>
    </ul>
  </div>
</div>

<div class="strip">
  <div class="content_box frame margin">
    <a name="introduction"></a>
    <h1>Introduction</h1>

    <h2>Index page</h2>

    <h2>Personal pages</h2>

    <h3>Notebook</h3>

    <h3>Greenhouses</h3>

    <h3>Channel page</h3>

    <h3>Tag page</h3>

    <h3>Account settings</h3>

    <h2>Global channel page</h2>

    <h2>Global tag page</h2>


  </div>
</div>

<div class="strip">
  <div class="content_box frame margin">

    <a name="editing"></a>
    <h1>Notebook</h1>

    <p>The notebook is the place where you can keep a trace of
    whatever comes to your mind: ideas, observations, etc. It's a
    place to write down what worked well in the garden and what
    didn't. It is also the place where you can ask questions to other
    people.</p>

    <p>The notes in the book are ordered in a chronological order. The
    notes at the top are the most recent ones.</p>

    <p>Other people can react to your notes. For example, they can
    reply to your questions, or provide tips and help. Each post can
    therefore evolve into a discussion thread.</p>

    <a name="editing:length"></a>
    <h2>Writing notes</h2>

    <p>To write a new note, you have to log in first. Once logged in,
    you should see a link called "Write a new note" at the top of your
    notebook. Clicking the link will open up an editor.</p>

    <a name="editing:length"></a>
    <h2>Message size limit</h2>

    <p>To keep posts short and to the point, we've limited their size
    to 2000 characters.</p>

    <a name="editing:photos"></a>
    <h2>Adding photos</h2>

    <p>For each note, you can join four images.</p>


    <a name="editing:links"></a>
    <h2>Linking to web sites</h2>

    <p>You can insert links to other web pages by inserting the
    address of the page between square brackets ("[" and "]"). Make
    sure that the address starts with http:// or https://. The
    address will be automatically converted to a clickable link. For
    example, [http://p2pfoodlab.net].</p>


    <a name="editing:sendto"></a>
    <h2>Sending messages to others</h2>

    <p>You can send a message to someone else by inserting her
    username in the post, preceded by the "@" sign. For example,
    @peter.</p>


    <a name="editing:hashtags"></a>
    <h2>Hashtags</h2>

    <p>Inside a note you can use hashtags to link notes to a topic. To
    use a hashtag, simply precede any word with the "#" sign. For
    example, #tomatoes.</p>
    
    <p>When your note is uploaded, the hashtag will be converted into
    a clickable link that points to all your notes that use the
    hashtag. It is also possible to views everyone's posts with the
    given tag.</p>


    <a name="editing:channels"></a>
    <h2>Channels</h2>

    <p>Several hashtags have a special meaning: they refer to
    predefined channels. Channels have their own dedicated web
    page. The table below summarises the currently defined channels
    and the associated hastags:</p>

    <div>
      <table class="channels">
        <tr>
          <td class="">Channel</td>
          <td class="">Hashtags</td>
        </tr>
        <?php 
           for ($i = 0; $i < count($this->channels); $i++) { 
           $c = $this->channels[$i];
        $tags = $c->tags;
        $s = "";
        for ($j = 0; $j < count($tags); $j++) { 
                          if ($j > 0) $s .= " &nbsp; ";
          $s .= "#" . $tags[$j];
          }
          ?>
        <tr>
          <td class=""><?php echo $c->title ?></td>
          <td class=""><?php echo $s ?></td>
        </tr>
        <?php 
           }
           ?>
      </table>
    </div>


  </div>
</div>


<div class="strip">
  <div class="content_box frame margin">

    <a name="settings"></a>
    <h1>Account settings</h1>


    <a name="settings:language"></a>
    <h2>Language settings</h2>

    <p>
      The Web site uses two language settings:

      <ul>
        <li>For the interface: The language used for the menus, the
          buttons, the help indications, and so on. The choice of
          language options will be limited by translations that we can
          handle. If you are willing to help us translate the
          interface in a new language, please contact us.</li>

        <li>For your notebook: The language used for the contents, in
          particular, for your notebook can be different than the
          language used for the interface. In fact, you can submit
          notes in any language you wish. You can also maintain
          several notebooks in different languages. This is handy for
          people who participate in several language groups. However,
          to make it easier to filter your notes based on the
          language, we ask you to select the ones that you intend to
          use.</li>
      </ul>
    </p>

  </div>
</div>


<div class="strip">
  <div class="content_box frame margin">
    <a name="sensorbox"></a>
    <h1>Greenhouses and datasets</h1>


  </div>
</div>


<div class="strip">
  <div class="content_box frame margin">
    <a name="opensensordata"></a>
    <h1>OpenSensorData.net</h1>


  </div>
</div>

