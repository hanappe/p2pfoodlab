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

    <script type="text/javascript"> 
      function submitForm() 
      {
          var visitor = document.forms['login'].visitor.value;
          var pw = document.forms['login'].pwhash.value;
          document.forms['login'].pwhash.value = hex_md5(visitor + pw);
          document.forms['login'].submit();
      }
    </script>

<?php if ($this->message): ?>
        <div class="strip">
            <div class="content_box message margin">
                <?php echo $this->message ?>
            </div>
        </div>
<?php endif; ?>

        <div class="strip">
            <div class="content_box frame">
              <form action="<?php url('login') ?>" method="post" name="login" onsubmit="return submitForm();" class="login">
                <input type="hidden" name="op" value="login" />
                <table class="login">
                  <tr>
                    <td class="label"><?php _p("Username") ?></td>
                    <td class="input"><input type="text" name="visitor" size="8" maxlength="32" value="<?php echo $this->username ?>" /></td>
                  </tr>
                  <tr>
                    <td class="label"><?php _p("Password") ?></td>
                    <td class="input">
                      <input type="password" name="pwhash" size="8" maxlength="30" value="" />
                    </td>
                  </tr>
                  <tr>
                    <td></td>
                    <td class="input">
                      <input type="submit" class="button" value="OK" />
                    </td>
                  </tr>
                  <tr>
                    <td></td>
                    <td class="input">
                      <?php _p("or") ?> <a href="<?php url('createaccount') ?>"><?php _p("create a new account") ?></a>.
                    </td>
                  </tr>
                </table>
                <input type="submit" style="display:none"/>
              </form>
            </div>
        </div>
