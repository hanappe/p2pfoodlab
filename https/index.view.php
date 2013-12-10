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

?>
<div class="strip" style="margin-top: 20px;">
  <div class="empty_box index_box fg_white frame">
    <a href="http://p2pfoodlab.net">
      <div class="index_entry"><?php _p("about P2P Food Lab") ?></div>                       
    </a>
    <img src="DSC03573.JPG" class="adjusted" />
  </div> 
</div> 


<div class="strip">
  <div class="empty_box index_box fg_white frame_blue">
    
    <form action="<?php url('login') ?>" method="post" name="login" class="qlogin"  style="vertical-align: bottom;">
      <input type="hidden" name="op" value="login" />
      <table class="qlogin">
        <tr>
          <td class="label large fg_white"><?php _p("username") ?></td>
          <td class="input"><input type="text" name="visitor" size="8" maxlength="32" value="<?php echo $this->username ?>" /></td>
        </tr>
        <tr>
          <td class="label large fg_white"><?php _p("password") ?></td>
          <td class="input">
            <input type="password" name="pwhash" size="8" maxlength="30" value="" />
          </td>
        </tr>
      </table>
      <input type="submit" style="position: absolute; left: -9999px; width: 1px; height: 1px;"/>
    </form><br/>
    <a href="createaccount" style="color: white; font-size: 75%;">You don't have an account? Create a new one.</a>
    <img src="DSC01408.JPG" class="adjusted" />
  </div>
</div> 

<div class="strip">
  <div class="empty_box index_box fg_white frame_green">
    <a href="<?php url('posts') ?>">
      <div class="index_entry"><?php _p("recent posts") ?></div>                       
    </a>
    <img src="DSC01421.JPG" class="adjusted" />
  </div>
</div> 

<div class="strip">     
  <div class="empty_box index_box fg_white frame_blue">
    <a href="http://p2pfoodlab.net">
      <div class="index_entry"><?php _p("meet the participants") ?></div>                       
    </a>
    <img src="DSC03517.JPG" class="adjusted" />
  </div> 
</div> 



<!--


<div class="strip">
  <div class="content_box frame clearfix margin">

<?php if ($lang->code == 'fr'): ?>
    <img src="<?php url('DSC00604.JPG') ?>" class="floatleft margin" width="240px"/>
    <p class="index">Bienvenue sur le site communautaire de P2P Food Lab.</p>
    <p class="index"><a href="http://p2pfoodlab.net" class="standout">Apprenez plus sur le projet P2P Food Lab.</a></p>
    <p class="index"><a href="<?php url('help') ?>" class="standout">Comment utiliser ce site ?</a></p>

<?php else: ?>

    <img src="<?php url('DSC00604.JPG') ?>" class="floatleft margin" width="240px"/>
    <p class="index">Welcome to the community web site of P2P Food Lab.</p>
    <p class="index"><a href="http://p2pfoodlab.net" class="standout">Learn more about the P2P Food Project.</a></p>
    <p class="index"><a href="<?php url('help') ?>" class="standout">How do I use this site?</a></p>
<?php endif; ?>

  </div> 
</div> 
  
<div class="strip">
  <div class="content_box">
    <p class="title"><?php _p("meet the participants") ?></p>
    <div class="frame margin">
<?php
          for ($i = 0; $i < count($this->accounts); $i++) {
                  $account = $this->accounts[$i];
                  $url = $base_url . "/people/" . $account->username;
                  echo "<a href=\"$url\" class=\"rightmargin\">" . $account->username . "</a>\n";
          }
?>       
    </div> 
  </div> 
</div> 



<div class="strip">
  <div class="content_box">
   <p class="title"><?php _p("browse the message channels") ?></p>
    <div class="margin frame">
<?php
          for ($i = 0; $i < count($this->channels); $i++) {
                  $channel = $this->channels[$i];
                  $url = $base_url . "/channel/" . $channel->id;
                  echo "<a href=\"$url\" class=\"rightmargin\">" . $channel->title . "</a>\n";
          }
?>       
    </div> 
  </div> 
</div> 

<div class="strip">
  <div class="content_box">
    <p class="title"><?php _p("read the most recent posts") ?></p>
  </div> 
</div> 

-->
