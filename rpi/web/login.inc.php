<?php
/*  
    P2P Food Lab Sensorbox

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
?><html>
  <head>
    <title>P2P Food Lab Sensorbox</title>
    <link rel="stylesheet" href="sensorbox.css" />
    <script src="md5.js" type="text/javascript"></script>
  </head>

  <body>
    <div class="content">

<script type="text/javascript"> 
function submitForm() 
{
        var salt = document.forms['login'].pwsalt.value;
        var pw = document.forms['login'].pwhash.value;
        document.forms['login'].pwsalt.value = "";
        document.forms['login'].pwhash.value = hex_md5(salt + hex_md5(pw));
        document.forms['login'].submit();
}
</script>

      <div class="panel">
        <form action="index.php" method="post" name="login" onsubmit="return submitForm();">
          <input type="hidden" name="op" value="login" />
          <input type="hidden" name="pwsalt" value="<?php echo $config->login->pwsalt ?>" />
          <table>
            <tr>
              <td class="label">Password</td>
              <td class="input">
                <input type="password" name="pwhash" size="8" maxlength="30" value="" />
              </td>
            </tr>
          </table>
          <input type="submit" style="display:none"/>
        </form>
      </div>

    </div>
  </body>
</html>
