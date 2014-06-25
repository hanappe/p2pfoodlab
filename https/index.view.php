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
    <div class="index_box">
      <img src="p2pfoodlab-logo-800x200.png" class="adjusted" />
    </div> 
</div> 

<div class="strip" style="margin-top: 20px;">
    <div class="index_box">
      <a href="http://p2pfoodlab.net" class="index_entry">
        <?php _p("about P2P Food Lab") ?>
      </a>
      <img src="DSC03573.JPG" class="adjusted" />
    </div> 
</div> 


<div class="strip">
  <div class="index_box fg_white">

    <script type="text/javascript"> 
      function submitForm() 
      {
          var visitor = document.forms['login'].visitor.value;
          var pw = document.forms['login'].pwhash.value;
          document.forms['login'].pwhash.value = hex_md5(visitor + pw);
          document.forms['login'].submit();
      }
    </script>
    
    <form action="<?php url('login') ?>" method="post" name="login" class="qlogin" onsubmit="return submitForm();" >
      <input type="hidden" name="op" value="login" />
      <table class="qlogin">
        <tr>
          <td class="label large fg_white"><?php _p("username") ?></td>
          <td class="input"><input type="text" name="visitor" size="8" maxlength="32" /></td>
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
  <div class="index_box">
    <a href="<?php url('posts') ?>" class="index_entry">
      <?php _p("recent posts") ?>
    </a>
    <img src="DSC01421.JPG" class="adjusted" />
  </div>
</div> 

<div class="strip">     
  <div class="index_box">
    <a href="<?php url('people') ?>" class="index_entry">
      <?php _p("meet the participants") ?>
    </a>
    <img src="DSC03517.JPG" class="adjusted" />
  </div> 
</div> 


