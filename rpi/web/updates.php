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

require_once "config.inc.php";
require_once "session.inc.php";

$json = file_get_contents("https://p2pfoodlab.net/sensorbox-versions.json");

if ($json === FALSE) {
        $error = "Failed to load https://p2pfoodlab.net/sensorbox-versions.json";
} else {

        $versions = json_decode($json);

        if ($versions === NULL) {
                $json_error = "";
                switch (json_last_error()) {
                case JSON_ERROR_NONE:
                        $json_error = 'No errors';
                        break;
                case JSON_ERROR_DEPTH:
                        $json_error = 'Maximum stack depth exceeded';
                        break;
                case JSON_ERROR_STATE_MISMATCH:
                        $json_error = 'Underflow or the modes mismatch';
                        break;
                case JSON_ERROR_CTRL_CHAR:
                        $json_error = 'Unexpected control character found';
                        break;
                case JSON_ERROR_SYNTAX:
                        $json_error = 'Syntax error, malformed JSON';
                        break;
                case JSON_ERROR_UTF8:
                        $json_error = 'Malformed UTF-8 characters, possibly incorrectly encoded';
                        break;
                default:
                        $json_error = 'Unknown error';
                        break;
                }
                $error = "Failed to parse the version data: " . $json_error;
        }
 }

function install_version($v)
{
        global $error, $config, $update_file;

        $contents = ("VERSION=" . $v->version . "\n"
                     . "DATE=" . $v->date . "\n"
                     . "CHECKSUM=" . $v->checksum . "\n"
                     . "URL=" . $v->url . "\n");

        $r = file_put_contents($update_file, $contents);
        if ($r === FALSE) {
                $error = "Failed to save the update file. Call for help!";
                return FALSE;
        } 
        
        $output = file_get_contents("http://127.0.0.1:10080/update/version");
        if ($output === FALSE) {
                $error = "Failed to run the low-level updater.";
                return FALSE;                
        }

        $config->version->string = $v->version;
        $config->version->date = $v->date;

        if (!save_config($config)) {
                $error = $config_error;
                return FALSE;
        }
        
        return TRUE;
}

$version = $_REQUEST['version'];
if (isset($version) && $version) {
        for ($i = 0; $i < count($versions); $i++) {
                $v = $versions[$i];
                if ($v->date == $version) {
                        install_version($v);
                        break;
                }
        }
 }

?><html>
  <head>
    <title>P2P Food Lab Sensorbox</title>
    <script src="md5.js" type="text/javascript"></script>
    <link rel="stylesheet" href="sensorbox.css" />
  </head>
  
  <body>
    
    <div class="content">
      
      <div class="header">P2P Food Lab Sensorbox</div>
      <div class="menubar">
        <div class="pagemenu">
          <a class="pagemenu" href="index.php">Status</a> - 
          <a class="pagemenu" href="configuration.php">Configuration</a> - 
          <a class="pagemenu" href="log.php">View log file</a> - 
          <a class="pagemenu" href="updates.php">Update software</a> - 
          <a class="pagemenu" href="index.php?op=logout">Logout</a> 
         </div>
      </div>
      
      <div class="main">
<?php 

if (isset($error)) 
        echo "        <div class='message'>$error</div>\n"; 
 else {
         echo "        <div>\n";
         echo "          <table>\n";
         echo "            <tr>\n";
         echo "              <td>Version</td>\n";
         echo "              <td>Date</td>\n";
         echo "              <td>Comment</td>\n";
         echo "              <td></td>\n";
         echo "            </tr>\n";
         
         for ($i = count($versions) - 1; $i >= 0; $i--) {
                 $v = $versions[$i];
                 if ($v->date) {
                         echo "            <tr>\n";
                         echo "              <td>" . $v->version . "</td>\n";
                         echo "              <td>" . $v->date . "</td>\n";
                         echo "              <td>" . $v->comment . "</td>\n";
                         if ($v->date == $config->version->date) {
                                 echo "              <td><a href='updates.php?version=" . $v->date . "'>Re-install</a></td>\n";
                         } else {
                                 echo "              <td><a href='updates.php?version=" . $v->date . "'>Install</a></td>\n";
                         }
                         echo "            </tr>\n";
                 }
         }
         echo "          </table>\n";
         echo "        </div>\n";
 } 

?>
      </div>
    </div>
  </body>
</html>



