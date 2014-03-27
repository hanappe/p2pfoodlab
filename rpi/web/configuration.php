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
require_once "error.inc.php";
require_once "session.inc.php";
require_once "update.inc.php";


function load_group() 
{
  global $osd_dir;

  $filename = $osd_dir . "/group.json";
  if (is_readable($filename)) {
    $json = file_get_contents($filename);
    if ($json != FALSE) {
      // FIXME: if the description or the unit changed, shouldn't we create a new definition?
      return json_decode($json);
    }
  }
  return FALSE;
}

$group = load_group();

$config->login->pwsalt = get_random_string(12);
if (!save_config($config)) {
        echo $config_error . "<br>";
        exit();
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
          <a class="pagemenu" href="data/">Browse data files</a> - 
          <a class="pagemenu" href="updates.php">Update software</a> - 
          <a class="pagemenu" href="index.php?op=poweroff">Power off</a> -
          <a class="pagemenu" href="index.php?op=logout">Logout</a> 
        </div>
      </div>
      
      <?php if ($msg): ?>
      <div class="message">
        <?php echo $msg . "<br/>\n" ?>
      </div>
      <?php endif; ?>

      <?php if ($system_output): ?>
      <div class="system_output">
        System output:<br/>
        <pre><?php echo $system_output . "\n" ?></pre>
      </div>
      <?php endif; ?>
    
      <div class="panel">
        <h1>Login</h1>

<script type="text/javascript"> 
function submitLogin() 
{
        var salt = document.forms['login'].pwsalt.value;
        var oldpw = document.forms['login'].oldpw.value;
        var pw = document.forms['login'].pwhash.value;
        var pw2 = document.forms['login'].pw2.value;
        if (pw != pw2) {
            alert("Please check the passwords");
            return;
        }
        if (pw == "") {
            alert("Please check the passwords");
            return;
        }
        document.forms['login'].pwsalt.value = "";
        document.forms['login'].oldpw.value = hex_md5(salt + hex_md5(oldpw));
        document.forms['login'].pwhash.value = hex_md5(pw);
        document.forms['login'].pw2.value = "";
        document.forms['login'].submit();
}
</script>

        <form action="configuration.php" method="post" name="login" onsubmit="return submitLogin();">
          <input type="hidden" name="op" value="update" />
          <input type="hidden" name="section" value="login" />
          <input type="hidden" name="pwsalt" value="<?php echo $config->login->pwsalt ?>" />
          <table>
            <tr>
              <td class="label">Old password</td>
              <td class="input"><input type="password" name="oldpw" size="20" maxlength="32" value="" /></td>
            </tr>
            <tr>
              <td class="label">New password</td>
              <td class="input"><input type="password" name="pwhash" size="20" maxlength="32" value="" /></td>
            </tr>
            <tr>
              <td class="label">Confirm password</td>
              <td class="input"><input type="password" name="pw2" size="20" maxlength="32" value="" /></td>
            </tr>
          </table>
          <input type="submit" class="button" value="Change password" />
        </form>
      </div>
      
      <div class="panel">
        <h1>General info</h1>

        <?php $general_config = $config->general; ?>

        <form action="configuration.php" method="post" name="general">
          <input type="hidden" name="op" value="update" />
          <input type="hidden" name="section" value="general" />
          <table>
            <tr>
              <td class="label">Name of sensorbox</td>
              <td class="input"><input type="text" name="name" size="20" maxlength="32" value="<?php echo $general_config->name ?>" /></td>
            </tr>
            <tr>
              <td class="label">Email (where to send status reports)</td>
              <td class="input"><input type="text" name="email" size="20" maxlength="32" value="<?php echo $general_config->email ?>" /></td>
            </tr>
            <tr>
              <td class="label">Timezone</td>
              <td class="input"><input type="text" name="timezone" size="20" maxlength="32" value="<?php echo $general_config->timezone ?>" /></td>
            </tr>
            <tr>
              <td class="label">Latitude</td>
              <td class="input"><input type="text" name="latitude" size="20" maxlength="32" value="<?php echo $general_config->latitude ?>" /></td>
            </tr>
            <tr>
              <td class="label">Longitude</td>
              <td class="input"><input type="text" name="longitude" size="20" maxlength="32" value="<?php echo $general_config->longitude ?>" /></td>
            </tr>
          </table>
          <input type="submit" class="button" value="Update general info" />
        </form>
      </div>
      
      <div class="panel">
        <h1>Power management</h1>

        <?php $power_config = $config->power; ?>

        <form action="configuration.php" method="post" name="power">
          <input type="hidden" name="op" value="update" />
          <input type="hidden" name="section" value="power" />
          <table>
            <tr>
               <td class="label" colspan="2">Turn on energy saving (shut down the Raspberry Pi between measurements)?</td>
            </tr>
            <tr>
               <td class="label">Energy saving?</td>
               <td class="input"><input type="checkbox" name="poweroff" <?php if ($power_config->poweroff == "yes") echo "checked" ?> value="yes" /></td>
            </tr>
          </table>
          <input type="submit" class="button" value="Update power management" />
        </form>
      </div>
      
      <div class="panel">
        <h1>Wired network</h1>
        
        <?php 
           $wired_config = $config->wired;
           $static_config = $wired_config->static;
           $dhcpd_config = $wired_config->dhcpserver;
        ?>
        <form action="configuration.php" method="post" name="wired">
          <input type="hidden" name="op" value="update" />
          <input type="hidden" name="section" value="wired" />
          
          <table>
            <tr>
              <td class="label">Network configuration</td>
              <td class="input">
                <select id="inet" name="inet">
                  <option value="dhcp" <?php if ($wired_config->inet == "dhcp") echo "selected" ?>>DHCP</option>
                  <option value="static" <?php if ($wired_config->inet == "static") echo "selected" ?>>Static address</option>
                </select>
              </td>
            </tr>
            <tr>
              <td class="label">IP address</td>
              <td class="input"><input type="text" name="address" size="20" maxlength="32" value="<?php echo $static_config->address ?>" /></td>
            </tr>
            <tr>
              <td class="label">Netmask</td>
              <td class="input"><input type="text" name="netmask" size="20" maxlength="32" value="<?php echo $static_config->netmask ?>" /></td>
            </tr>
            <tr>
              <td class="label">Gateway</td>
              <td class="input"><input type="text" name="gateway" size="20" maxlength="32" value="<?php echo $static_config->gateway ?>" /></td>
            </tr>
            <tr>
               <td class="label">Enable DHCP server (requires static IP address)?</td>
               <td class="input"><input type="checkbox" name="enable" <?php if ($dhcpd_config->enable == "yes") echo "checked" ?> value="yes" /></td>
            </tr>
            <tr>
              <td class="label">DHCP subnet</td>
              <td class="input"><input type="text" name="subnet" size="20" maxlength="32" value="<?php echo $dhcpd_config->subnet ?>" /></td>
            </tr>
            <tr>
              <td class="label">DHCP range from</td>
              <td class="input"><input type="text" name="from" size="20" maxlength="32" value="<?php echo $dhcpd_config->from ?>" /></td>
            </tr>
            <tr>
              <td class="label">DHCP range to</td>
              <td class="input"><input type="text" name="to" size="20" maxlength="32" value="<?php echo $dhcpd_config->to ?>" /></td>
            </tr>

           </table>
          <input type="submit" class="button" value="Update network config" />
        </form>
      </div>
  

      <div class="panel">
        <h1>WiFi network</h1>
        
        <?php 
           $wifi_config = $config->wifi;
        ?>
        <form action="configuration.php" method="post" name="wifi">
          <input type="hidden" name="op" value="update" />
          <input type="hidden" name="section" value="wifi" />
          
          <table>
            <tr>
              <td class="label">Enable WiFi?</td>
              <td class="input"><input type="checkbox" name="enable" <?php if ($wifi_config->enable == "yes") echo "checked" ?> value="yes" /></td>
            </tr>
            <tr>
              <td class="label">SSID</td>
              <td class="input"><input type="text" name="ssid" size="20" maxlength="32" value="<?php echo $wifi_config->ssid ?>" /></td>
            </tr>
            <tr>
              <td class="label">Password</td>
              <td class="input"><input type="text" name="password" size="20" maxlength="32" value="<?php echo $wifi_config->password ?>" /></td>
            </tr>
          </table>
          <input type="submit" class="button" value="Update network config" />
        </form>
      </div>
      
      <div class="panel">
        <h1>GSM network</h1>
        
        <?php 
           $gsm_config = $config->gsm;
        ?>
        <form action="configuration.php" method="post" name="gsm">
          <input type="hidden" name="op" value="update" />
          <input type="hidden" name="section" value="gsm" />
          
          <table>
            <tr>
              <td class="label">Enable GSM?</td>
              <td class="input"><input type="checkbox" name="enable" <?php if ($gsm_config->enable == "yes") echo "checked" ?> value="yes" /></td>
            </tr>
            <tr>
              <td class="label">PIN</td>
              <td class="input"><input type="text" name="pin" size="20" maxlength="32" value="<?php echo $gsm_config->pin ?>" /></td>
            </tr>
            <tr>
              <td class="label">APN</td>
              <td class="input"><input type="text" name="apn" size="20" maxlength="32" value="<?php echo $gsm_config->apn ?>" /></td>
            </tr>
            <tr>
              <td class="label">Username</td>
              <td class="input"><input type="text" name="username" size="20" maxlength="32" value="<?php echo $gsm_config->username ?>" /></td>
            </tr>
            <tr>
              <td class="label">Password</td>
              <td class="input"><input type="text" name="password" size="20" maxlength="32" value="<?php echo $gsm_config->password ?>" /></td>
            </tr>
            <tr>
              <td class="label">Phone</td>
              <td class="input"><input type="text" name="phone" size="20" maxlength="32" value="<?php echo $gsm_config->phone ?>" /></td>
            </tr>
            <tr>
              <td class="label">Baud</td>
              <td class="input"><input type="text" name="baud" size="20" maxlength="32" value="<?php echo $gsm_config->baud ?>" /></td>
            </tr>
          </table>
          <input type="submit" class="button" value="Update network config" />
        </form>
      </div>
      
      <div class="panel">
        <h1>Photostream</h1>

        <div class="testimage">
<script type="text/javascript"> 
var count = 0;
function testCamera() 
{
        var img = document.getElementById('testimage');
        if (img) img.src = "camera.php?t=" + (count++);
}
</script>
          <img src="white.jpg" width="200px" name="testimage" id="testimage" /><br/>
          <a class="testimage" href="javascript:void(0)" onclick="testCamera()">Grab camera image</a>
        </div>
        
        <?php $camera_config = $config->camera; ?>
        <form action="configuration.php" method="post" name="camera">
          <input type="hidden" name="op" value="update" />
          <input type="hidden" name="section" value="camera" />
          
          <table>
            <tr>
               <td class="label">Enable camera?</td>
               <td class="input"><input type="checkbox" name="enable" <?php if ($camera_config->enable == "yes") echo "checked" ?> value="yes" /></td>
            </tr>
            <tr>
              <td class="label">Device</td>
              <td class="input"><input type="text" name="device" size="20" maxlength="32" value="<?php echo $camera_config->device ?>" /></td>
            </tr>

            <tr>
              <td class="label">Image size:</td>
              <td class="input">
                <select id="size" name="size">
                  <option value="320x240" <?php if ($camera_config->size == "320x240") echo "selected" ?>>320x240</option>
                  <option value="640x480" <?php if ($camera_config->size == "640x480") echo "selected" ?>>640x480</option>
                  <option value="960x720" <?php if ($camera_config->size == "960x720") echo "selected" ?>>960x720</option>
                  <option value="1024x768" <?php if ($camera_config->size == "1024x768") echo "selected" ?>>1024x768</option>
                  <option value="1280x720" <?php if ($camera_config->size == "1280x720") echo "selected" ?>>1280x720</option>
                  <option value="1280x960" <?php if ($camera_config->size == "1280x960") echo "selected" ?>>1280x960</option>
                  <option value="1920x1080" <?php if ($camera_config->size == "1920x1080") echo "selected" ?>>1920x1080</option>
                </select>
              </td>
            </tr>

            <tr>
              <td class="label">Take a photo:</td>
              <td class="input">
                <select id="update" name="update">
                  <option value="fixed" <?php if ($camera_config->update == "fixed") echo "selected" ?>>at fixed times</option>
                  <option value="periodical" <?php if ($camera_config->update == "periodical") echo "selected" ?>>periodically</option>
                </select>
              </td>
            </tr>

            <tr>
              <td class="label">Fixed time 1</td>
              <td class="input">
                   <input type="text" name="h0" size="2" maxlength="2" value="<?php echo $camera_config->fixed[0]->h ?>" />:
                   <input type="text" name="m0" size="2" maxlength="2" value="<?php echo $camera_config->fixed[0]->m ?>" />
              </td>
            </tr>
    
            <tr>
              <td class="label">Fixed time 2</td>
              <td class="input">
                   <input type="text" name="h1" size="2" maxlength="2" value="<?php echo $camera_config->fixed[1]->h ?>" />:
                   <input type="text" name="m1" size="2" maxlength="2" value="<?php echo $camera_config->fixed[1]->m ?>" />
              </td>
            </tr>
    
            <tr>
              <td class="label">Fixed time 3</td>
              <td class="input">
                   <input type="text" name="h2" size="2" maxlength="2" value="<?php echo $camera_config->fixed[2]->h ?>" />:
                   <input type="text" name="m2" size="2" maxlength="2" value="<?php echo $camera_config->fixed[2]->m ?>" />
              </td>
            </tr>
    
            <tr>
              <td class="label">Fixed time 4</td>
              <td class="input">
                   <input type="text" name="h3" size="2" maxlength="2" value="<?php echo $camera_config->fixed[3]->h ?>" />:
                   <input type="text" name="m3" size="2" maxlength="2" value="<?php echo $camera_config->fixed[3]->m ?>" />
              </td>
            </tr>
    
            <tr>
              <td class="label">Period</td>
              <td class="input"><input type="text" name="period" size="4" maxlength="4" value="<?php echo $camera_config->period ?>" /> minutes</td>
            </tr>
    
          </table>
          <input type="submit" class="button" value="Update camera config" />
        </form>
      </div>

      <div class="panel">
        <h1>Sensors</h1>
        
<?php
$sensors_config = $config->sensors; 
if (isset($sensors_config->rht03_1)
    && !isset($sensors_config->trh))
        $sensors_config->trh = $sensors_config->rht03_1;

if (isset($sensors_config->rht03_2)
    && !isset($sensors_config->trhx))
        $sensors_config->trhx = $sensors_config->rht03_2;

?>
        <form action="configuration.php" method="post" name="sensors">
          <input type="hidden" name="op" value="update" />
          <input type="hidden" name="section" value="sensors" />
          
          <table>
            <tr>
              <td class="label">Enable internal temperature and humidity sensor (RHT03)?</td>
              <td class="input"><input type="checkbox" name="trh" <?php if ($sensors_config->trh == "yes") echo "checked" ?> value="yes" /></td>
            </tr>
            <tr>
              <td class="label">Enable external temperature and humidity sensor (RHT03)?</td>
              <td class="input"><input type="checkbox" name="trhx" <?php if ($sensors_config->trhx == "yes") echo "checked" ?> value="yes" /></td>
            </tr>
            <tr>
              <td class="label">Enable luminosity sensor?</td>
              <td class="input"><input type="checkbox" name="light" <?php if ($sensors_config->light == "yes") echo "checked" ?> value="yes" /></td>
            </tr>
            <tr>
              <td class="label">Enable USB battery level sensor?</td>
              <td class="input"><input type="checkbox" name="usbbat" <?php if ($sensors_config->usbbat == "yes") echo "checked" ?> value="yes" /></td>
            </tr>

            <tr>
              <td class="label">Update sensor values every:</td>
              <td class="input">
                <select id="update" name="update">
                  <option value="1" <?php if ($sensors_config->update == "1") echo "selected" ?>>1 min</option>
                  <option value="5" <?php if ($sensors_config->update == "5") echo "selected" ?>>5 min</option>
                  <option value="10" <?php if ($sensors_config->update == "10") echo "selected" ?>>10 min</option>
                  <option value="15" <?php if ($sensors_config->update == "15") echo "selected" ?>>15 min</option>
                  <option value="30" <?php if ($sensors_config->update == "30") echo "selected" ?>>30 min</option>
                  <option value="60" <?php if ($sensors_config->update == "60") echo "selected" ?>>60 min</option>
                  <option value="120" <?php if ($sensors_config->update == "120") echo "selected" ?>>120 min</option>
                </select>
              </td>
            </tr>

            <tr>
              <td class="label">Upload data:</td>
              <td class="input">
                <select id="upload" name="upload">
                  <option value="fixed" <?php if ($sensors_config->upload == "fixed") echo "selected" ?>>at fixed times</option>
                  <option value="periodical" <?php if ($sensors_config->upload == "periodical") echo "selected" ?>>periodically</option>
                </select>
              </td>
            </tr>

            <tr>
              <td class="label">Fixed time 1</td>
              <td class="input">
                   <input type="text" name="h0" size="2" maxlength="2" value="<?php echo $sensors_config->fixed[0]->h ?>" />:
                   <input type="text" name="m0" size="2" maxlength="2" value="<?php echo $sensors_config->fixed[0]->m ?>" />
              </td>
            </tr>
    
            <tr>
              <td class="label">Fixed time 2</td>
              <td class="input">
                   <input type="text" name="h1" size="2" maxlength="2" value="<?php echo $sensors_config->fixed[1]->h ?>" />:
                   <input type="text" name="m1" size="2" maxlength="2" value="<?php echo $sensors_config->fixed[1]->m ?>" />
              </td>
            </tr>
    
            <tr>
              <td class="label">Fixed time 3</td>
              <td class="input">
                   <input type="text" name="h2" size="2" maxlength="2" value="<?php echo $sensors_config->fixed[2]->h ?>" />:
                   <input type="text" name="m2" size="2" maxlength="2" value="<?php echo $sensors_config->fixed[2]->m ?>" />
              </td>
            </tr>
    
            <tr>
              <td class="label">Fixed time 4</td>
              <td class="input">
                   <input type="text" name="h3" size="2" maxlength="2" value="<?php echo $sensors_config->fixed[3]->h ?>" />:
                   <input type="text" name="m3" size="2" maxlength="2" value="<?php echo $sensors_config->fixed[3]->m ?>" />
              </td>
            </tr>
    
            <tr>
              <td class="label">Upload period</td>
              <td class="input"><input type="text" name="period" size="4" maxlength="4" value="<?php echo $sensors_config->period ?>" /> minutes</td>
            </tr>
    
          </table>
          <input type="submit" class="button" value="Update sensors config" />
        </form>

<?php if ($group): ?>
	<p><a href="http://opensensordata.net/viewgroup/<?php echo $group->id ?>.csv">View data at OpenSensorData.net</a></p>
<?php endif; ?>
     </div>
      

      <div class="panel">
        <h1>OpenSensorData.net</h1>    
        <?php 
             $osd_config = $config->opensensordata; 
             /*
             if ($osd_config->username) {
                echo "Account name: " . $osd_config->username . "<br/>\n";
             } else {
                echo "Click update to download the account data.<br/>\n";
             } */
        ?>
        <form action="configuration.php" method="post" name="opensensordata">
          <input type="hidden" name="op" value="update" />
          <input type="hidden" name="section" value="opensensordata" />
          
          <table>
            <tr>
              <td class="label">Account key</td>
              <td class="input"><input type="text" name="key" size="32" maxlength="36" value="<?php echo $osd_config->key ?>" /></td>
            </tr>
          </table>
          <input type="submit" class="button" value="Update OpenSensorData config" />
        </form>        
      </div>


      <div class="panel">
        <h1>SSH</h1>    
        <?php $ssh_config = $config->ssh; ?>
        <form action="configuration.php" method="post" name="ssh">
          <input type="hidden" name="op" value="update" />
          <input type="hidden" name="section" value="ssh" />
          
          <table>
            <tr>
              <td class="label">Key 1</td>
              <td class="input"><input type="text" name="key1" size="40" maxlength="2048" value="<?php echo $ssh_config->key1 ?>" /></td>
            </tr>
            <tr>
              <td class="label">Key 2</td>
              <td class="input"><input type="text" name="key2" size="40" maxlength="2048" value="<?php echo $ssh_config->key2 ?>" /></td>
            </tr>
            <tr>
              <td class="label">Key 3</td>
              <td class="input"><input type="text" name="key3" size="40" maxlength="2048" value="<?php echo $ssh_config->key3 ?>" /></td>
            </tr>
          </table>
          <input type="submit" class="button" value="Update SSH config" />
        </form>        
      </div>

    </div>      
  </body>
</html>
