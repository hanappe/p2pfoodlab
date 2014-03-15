<?php /* 

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

  // xxx

require_once "config.inc.php";
require_once "opensensordata.inc.php";

$system_output = FALSE;
$msg = "";

function update_general()
{
        global $config_error, $msg;

        $tmp = load_config();
        if ($tmp === FALSE) {
                $msg .= $config_error;
                return NULL;
        }

        $general_config = $tmp->general;
        if (!$general_config) {
                $general_config = new StdClass();
                $tmp->general = $general_config;
                $general_config->name = "";
                $general_config->email = "";
                $general_config->timezone = "";
                $general_config->latitude = "";
                $general_config->longitude = "";
                if (!save_config($tmp)) {
                        echo $config_error . "<br>";
                        return NULL;
                }
        }

        $name = $_REQUEST['name'];
        if (!preg_match('/^[a-zA-Z][a-zA-Z0-9_-]{0,}$/', $name)) {
                $msg .= "Invalid name. Please use only small or capital letters, hyphens ('-') and underscores.";
                return NULL;
        }
        $general_config->name = $name;

        $email = $_REQUEST['email'];
        if (!filter_var($email, FILTER_VALIDATE_EMAIL)) {
                $msg .= "Invalid email address: " . $email;
                return NULL;                
        }
        $general_config->email = $email;

        $timezone = $_REQUEST['timezone'];
        if (!is_numeric($timezone)) {
                $msg .= "Invalid timezone: " . $timezone;
                return NULL;                
        }
        $general_config->timezone = $timezone;

        $latitude = $_REQUEST['latitude'];
        if (!is_numeric($latitude)) {
                $msg .= "Invalid latitude: " . $latitude;
                return NULL;                
        }
        $general_config->latitude = $latitude;

        $longitude = $_REQUEST['longitude'];
        if (!is_numeric($longitude)) {
                $msg .= "Invalid longitude: " . $longitude;
                return NULL;                
        }
        $general_config->longitude = $longitude;

        if (!save_config($tmp)) {
                echo $config_error . "<br>";
                return NULL;
        }

        return $tmp; 
}

function update_login()
{
        global $config_error, $msg;

        $tmp = load_config();
        if ($tmp === FALSE) {
                $msg .= $config_error;
                return NULL;
        }

        $login_config = $tmp->login;
        if (!$login_config) {
                $login_config = new StdClass();
                $tmp->login = $login_config;
                $login_config->pwsalt = "";
                $login_config->pwhash = "";
                if (!save_config($tmp)) {
                        echo $config_error . "<br>";
                        return NULL;
                }
        }

        $remote_hash = $_REQUEST['oldpw'];
        if (!preg_match('/^[0-9a-z]{32,32}$/', $remote_hash)) {
                $msg .= "Hmmm, weird characters in the old password hash...";
                return NULL;
        }

        $local_hash = md5($login_config->pwsalt . $login_config->pwhash);
        if ($local_hash != $remote_hash) {
                $msg .= "The old password was not correct.";
                return NULL;
        }

        /*
        echo "remote hash = " . $remote_hash . "<br>\n";
        echo "salt = " . $login_config->pwsalt . "<br>\n";
        echo "pshash = " . $login_config->pwhash . "<br>\n";
        echo "local hash = " . $local_hash . "<br>\n";
        */

        $pwhash = $_REQUEST['pwhash'];
        //echo "new hash = " . $pwhash . "<br>\n";
        if (!preg_match('/^[0-9a-z]{32,32}$/', $pwhash)) {
                $msg .= "Hmmm, weird characters in the password hash...";
                return NULL;
        }
        $login_config->pwhash = $pwhash;

        if (!save_config($tmp)) {
                echo $config_error . "<br>";
                return NULL;
        }

        return $tmp; 
}

function write_network_interfaces($config)
{
        global $interfaces_file, $msg;

        $wired_config = $config->wired;

        $contents = ("auto lo\n"
                       . "iface lo inet loopback\n"
                       . "\n");

        if ($wired_config->inet == "dhcp") {
                $contents .= ("auto eth0\n"
                                . "allow-hotplug eth0\n"
                                . "iface eth0 inet dhcp\n"
                                . "\n");
        } else if ($wired_config->inet == "static") {
                $static_config = $wired_config->static;
                $contents .= ("auto eth0\n"
                                . "iface eth0 inet static\n"
                                . "        address " . $static_config->address . "\n" 
                                . "        netmask " . $static_config->netmask . "\n" 
                                /* . "        gateway " . $static_config->gateway . "\n"  */
                                . "\n");
        }

        $wifi_config = $config->wifi;

        if ($wifi_config->enable == "yes") {
                $contents .= ("auto wlan0\n"
                              . "allow-hotplug wlan0\n"
                              . "iface wlan0 inet dhcp\n"
                              . "      wpa-ssid \"" . $wifi_config->ssid . "\"\n"
                              . "      wpa-psk \"" . $wifi_config->password . "\"\n"
                              . "\n");
        }

        $gsm_config = $config->gsm;

        if ($gsm_config->enable == "yes") {
                $contents .= ("iface ppp0 inet wvdial\n"
                                . "      provider gsmmodem\n"
                                . "\n");
        }

        $err = file_put_contents($interfaces_file, $contents);
        if ($err === FALSE) {
                $msg .= "Failed to save the network interfaces file. Call for help!";
                return FALSE;
        }

        return TRUE;
}

function write_dhcp_config($config)
{
        global $dhcpd_file, $msg;

        $wired_config = $config->wired;
        if (!$wired_config) {
                $msg .= "While writing DHCP server config: couldn't find wired config.<br>";
                return FALSE;
        }
        
        $static_config = $wired_config->static;
        if (!$static_config) {
                $msg .= "While writing DHCP server config: couldn't find static wired config.<br>";
                return FALSE;                
        }

        $dhcpd_config = $wired_config->dhcpserver;
        if (!$dhcpd_config) {
                $msg .= "While writing DHCP server config: couldn't find DHCP settings.<br>";
                return FALSE;                
        }

        if ($dhcpd_config->enable == "yes") {
                $contents = ("ddns-update-style none;\n"
                             . "option domain-name \"p2pfoodlab.lan\";\n"
                             . "option domain-name-servers " . $static_config->gateway . ";\n"
                             . "default-lease-time 3600;\n"
                             . "max-lease-time 7200;\n"
                             . "authoritative;\n"
                             . "log-facility local7;\n"
                             . "\n"
                             . "subnet " . $dhcpd_config->subnet . " netmask " . $static_config->netmask . " {\n"
                             . "  range " . $dhcpd_config->from . " " . $dhcpd_config->to . ";\n"
                             . "  option routers " . $static_config->gateway . ";\n"
                             . "}\n"
                             . "\n");
                
                $err = file_put_contents($dhcpd_file, $contents);
                if ($err === FALSE) {
                        $msg .= "Failed to save the DHCP server file. Call for help!";
                        return FALSE;
                }
        } else {
                // if (file exists)
                //unlink($dhcpd_file);

                // FIXME: For some reason, unlink doesn't work. Creating an empty file instead.
                $err = file_put_contents($dhcpd_file, "");
                if ($err === FALSE) {
                        $msg .= "Failed to save the DHCP server configuration file. Call for help!";
                        return FALSE;
                }
        }

        return TRUE;
}

function write_wvdial_config($config)
{
        global $wvdial_file, $msg;

        $gsm_config = $config->gsm;

        //echo "write_wvdial_config gsm_config->enable=" . $gsm_config->enable . "<br>\n";

        if ($gsm_config->enable == "yes") {
                $contents = ("[Dialer gsmmodem]\n"
                             . "Modem = /dev/ttyUSB0\n"
                             . "Modem Type = Analog Modem\n"
                             . "ISDN = 0\n"
                             . "Baud = " . $gsm_config->baud . "\n"
                             . "Username = " . $gsm_config->username . "\n"
                             . "Password = " . $gsm_config->password . "\n"
                             . "Init1 = ATZ\n"
                             . "Init2 = AT&F E1 V1 X1 &D2 &C1 S0=0\n"
                             . "Dial Attempts = 1\n"
                             . "Phone = " . $gsm_config->phone . "\n"
                             . "Stupid Mode = 1\n"
                             . "Init3 = AT+CGDCONT=1,\"IP\",\"" . $gsm_config->apn . "\"\n");
                
                if ($gsm_config->pin != "") 
                        $contents .= "Init4 = AT+CPIN=" . $gsm_config->pin . "\n";

                $contents .= "\n";

                $err = file_put_contents($wvdial_file, $contents);
                if ($err === FALSE) {
                        $msg .= "Failed to save the wvdial configuration file. Call for help!";
                        return FALSE;
                }
        } else {
                // if (file exists)
                //echo "Unlinking " . $wvdial_file . "<br>\n";
                //$success = unlink($wvdial_file);
                //echo "Unlinking " . $wvdial_file . ": " . $success . "<br>\n";

                // FIXME: For some reason, unlink doesn't work. Creating an empty file instead.
                $err = file_put_contents($wvdial_file, "");
                if ($err === FALSE) {
                        $msg .= "Failed to save the wvdial configuration file. Call for help!";
                        return FALSE;
                }
        }

        return TRUE;
}

function call_update_daemon($url)
{
        //echo "call_low_level_network_update: http://127.0.0.1:10080/update/network/" . $iface . "<br>\n";
        //error_reporting(~0); ini_set('display_errors', 1);

        $context = stream_context_create(array("http"
                                               => array(
                                                        "method" => "GET",
                                                        "header" => "Accept: xml/*, text/*, */*\r\n",
                                                        "ignore_errors" => false,
                                                        "timeout" => 120,
                                                        )
                                               ));
                                         
        $output = file_get_contents($url, false, $context);

        //var_dump($http_response_header);        
        //echo "http://127.0.0.1:10080/update/network/" . $iface . ": <br><pre>"; var_dump($output); echo "</pre><br>";

        if ($output === FALSE) {
                return FALSE;
        }
        return $output;
}

function call_low_level_network_update($iface)
{
        return call_update_daemon("http://127.0.0.1:10080/update/network/" . $iface);
}

function update_network($config, $if)
{
        //echo "update_network<br>\n";

        if (!write_network_interfaces($config))
                return FALSE;

        if (!write_dhcp_config($config))
                return FALSE;

        if (!write_wvdial_config($config))
                return FALSE;

        $output = call_low_level_network_update($if); 
        if ($output == FALSE) 
                return FALSE;

        return $output;
}

function update_wifi()
{
        global $config_error, $msg, $system_output;

        $tmp = load_config();
        if ($tmp === FALSE) {
                $msg .= $config_error;
                return NULL;
        }

        $wifi_config = $tmp->wifi;

        $enable = $_REQUEST['enable'];
        $wifi_config->enable = ($enable == "yes")? "yes" : "no";

        $ssid = $_REQUEST['ssid'];
        //echo "SSID $ssid<br>\n";
        if (!preg_match('/^[a-zA-Z0-9][a-zA-Z0-9_ @().-]{0,31}$/', $ssid)) {
                //if (!preg_match('/^[a-zA-Z0-9][a-zA-Z!#;\-_=%&()@<> |.:]{0,31}$/', $ssid)) {
                $msg .= "Hmmm, weird characters in the SSID. Please check again. If you think the SSID is valid, send us a bug report...";
                return NULL;
        }
        $wifi_config->ssid = $ssid;

        $password = $_REQUEST['password'];
        // FIXME: validate wifi password
        $wifi_config->password = $password;

        if (!save_config($tmp)) {
                echo $config_error . "<br>";
                return NULL;
        }

        $output = update_network($tmp, "wifi");
        if ($output == FALSE) {
                $msg .= "System update failed.<br>";
                return NULL;
        }
        $system_output = $output;

        return $tmp;
}

function update_wired()
{
        global $config_error, $msg, $system_output;

        $tmp = load_config();
        if ($tmp === FALSE) {
                $msg .= $config_error;
                return NULL;
        }

        $wired_config = $tmp->wired;

        $inet = $_REQUEST['inet'];
        if (($inet != "dhcp") && ($inet != "static")) {
                $msg .= "Invalid Internet configuration: " . $inet;
                return NULL;
        }
        $wired_config->inet = $inet;

        if ($inet == "static") {        
                $address = $_REQUEST['address'];
                if (!filter_var($address, FILTER_VALIDATE_IP, array('flags' => FILTER_FLAG_IPV4))) {
                        $msg .= "Invalid IPv4 address: " . $address;
                        return NULL;                
                }

                $netmask = $_REQUEST['netmask'];
                /* FIXME: filter_var doesn't work for netmask
                if (!filter_var($netmask, FILTER_VALIDATE_IP, array('flags' => FILTER_FLAG_IPV4))) {
                        $msg .= "Invalid IPv4 address: " . $netmask;
                        return NULL;                
                }
                */

                $gateway = $_REQUEST['gateway'];
                if (!filter_var($gateway, FILTER_VALIDATE_IP, array('flags' => FILTER_FLAG_IPV4))) {
                        $msg .= "Invalid IPv4 address: " . $gateway;
                        return NULL;                
                }

                $static_config = $wired_config->static;
                if (!$static_config) {
                        $static_config = new StdClass();
                        $wired_config->static = $static_config;
                        if (!save_config($tmp)) {
                                echo $config_error . "<br>";
                                return NULL;
                        }
                }

                $static_config->address = $address;
                $static_config->netmask = $netmask;
                $static_config->gateway = $gateway;

                $enable = $_REQUEST['enable'];

                $subnet = $_REQUEST['subnet'];
                if (!filter_var($subnet, FILTER_VALIDATE_IP, array('flags' => FILTER_FLAG_IPV4))) {
                        $msg .= "Invalid IPv4 address: " . $subnet;
                        return NULL;                
                }

                $from = $_REQUEST['from'];
                if (!filter_var($from, FILTER_VALIDATE_IP, array('flags' => FILTER_FLAG_IPV4))) {
                        $msg .= "Invalid IPv4 address: " . $netmask;
                        return NULL;                
                }

                $to = $_REQUEST['to'];
                if (!filter_var($to, FILTER_VALIDATE_IP, array('flags' => FILTER_FLAG_IPV4))) {
                        $msg .= "Invalid IPv4 address: " . $netmask;
                        return NULL;                
                }

                $dhcpd_config = $wired_config->dhcpserver;
                if (!$dhcpd_config) {
                        $dhcpd_config = new StdClass();
                        $wired_config->dhcpserver = $dhcpd_config;
                        if (!save_config($tmp)) {
                                echo $config_error . "<br>";
                                return NULL;
                        }
                }

                $dhcpd_config->enable = ($enable == "yes")? "yes" : "no";
                $dhcpd_config->subnet = $subnet;
                $dhcpd_config->from = $from;
                $dhcpd_config->to = $to;
        }

        if (!save_config($tmp)) {
                echo $config_error . "<br>";
                return NULL;
        }

        $output = update_network($tmp, "wired");
        if ($output == FALSE) {
                $msg .= "System update failed.<br>";
                return NULL;
        }
        $system_output = $output;

        return $tmp;
}

function update_gsm()
{
        global $config_error, $msg, $system_output;

        $tmp = load_config();
        if ($tmp === FALSE) {
                $msg .= $config_error;
                return NULL;
        }

        $gsm_config = $tmp->gsm;
        if (!$gsm_config) {
                $gsm_config = new StdClass();
                $tmp->gsm = $gsm_config;
                if (!save_config($tmp)) {
                        echo $config_error . "<br>";
                        return NULL;
                }
        }

        $enable = $_REQUEST['enable'];
        $gsm_config->enable = ($enable == "yes")? "yes" : "no";

        $pin = $_REQUEST['pin'];
        if (!preg_match('/^[0-9]{0,12}$/', $pin)) {
                $msg .= "Hmmm, weird characters in the PIN. Please check again. If you think the PIN is valid, send us a bug report...";
                return NULL;
        }
        $gsm_config->pin = $pin;

        $apn = $_REQUEST['apn'];
        if (!preg_match('/^[a-zA-Z0-9-_.]{0,64}$/', $apn)) {
                $msg .= "Hmmm, weird characters in the APN. Please check again. If you think the APN is valid, send us a bug report...";
                return NULL;
        }
        $gsm_config->apn = $apn;

        $username = $_REQUEST['username'];
        if (!preg_match('/^[a-zA-Z0-9-_.]{0,64}$/', $apn)) {
                $msg .= "Hmmm, weird characters in the username. Please check again. If you think the entry is valid, send us a bug report...";
                return NULL;
        }
        $gsm_config->username = $username;

        $password = $_REQUEST['password'];
        // FIXME: validate password
        $gsm_config->password = $password;

        $phone = $_REQUEST['phone'];
        // FIXME: validate phone
        $gsm_config->phone = $phone;

        $baud = $_REQUEST['baud'];
        if (!preg_match('/^[0-9]{4,8}$/', $baud)) {
                $msg .= "Hmmm, weird characters in the Baud rate. Please check again. If you think the value is correct, send us a bug report...";
                return NULL;
        }
        $gsm_config->baud = $baud;

        if (!save_config($tmp)) {
                echo $config_error . "<br>";
                return NULL;
        }

        $output = update_network($tmp, "gsm");
        if ($output == FALSE) {
                $msg .= "System update failed.<br>";
                return NULL;
        }
        $system_output = $output;

        return $tmp;
}

function update_camera()
{
        global $config_error, $msg, $system_output;

        $tmp = load_config();
        if ($tmp === FALSE) {
                $msg .= $config_error;
                return NULL;
        }

        $camera_config = $tmp->camera;
        if (!$camera_config) {
                $camera_config = new StdClass();
                $tmp->camera = $camera_config;
                if (!save_config($tmp)) {
                        echo $config_error . "<br>";
                        return NULL;
                }
        }

        $enable = $_REQUEST['enable'];
        $camera_config->enable = ($enable == "yes")? "yes" : "no";

        $device = $_REQUEST['device'];
        if (!preg_match('|^/dev/video[0-9]$|', $device)) {
                $msg .= "Invalid device. (We're currently only allowing /dev/videoX device names. If this is too restrictive, please contact us.)";
                return NULL;
        }
        $camera_config->device = $device;

        $size = $_REQUEST['size'];
        if (!preg_match('/^[0-9]{3,4}x[0-9]{3,4}$/', $size)) {
                $msg .= "Invalid image size.";
                return NULL;
        }
        $camera_config->size = $size;

        $update = $_REQUEST['update'];
        if (($update != "fixed") && ($update != "periodical")) {
                $msg .= "Invalid value for the update. Only 'fixed' or 'periodical' are allowed.";
                return NULL;
        }
        $camera_config->update = $update;

        $period = $_REQUEST['period'];
        if (($period == "") && !is_numeric($period) || ($period < 1) || ($period > 60)) {
                $msg .= "Invalid value for the period. It should be a positive number between 1 and 60 minutes.";
                return NULL;
        }
        $camera_config->period = $period;

        for ($i = 0; $i < 4; $i++) {
                $hi = "h" . $i;
                $mi = "m" . $i;

                $h = $_REQUEST[$hi];
                if (($h != "") && (!is_numeric($h) || ($h < 0) || ($h > 23))) {
                        $msg .= "Invalid hour for the fixed times ('" . $h . "').";
                        return NULL;
                }
                $m = $_REQUEST[$mi];
                if (($m != "") && (!is_numeric($m) || ($m < 0) || ($m > 59))) {
                        $msg .= "Invalid minute for the fixed times ('" . $m . "').";
                        return NULL;
                }
                $camera_config->fixed[$i]->h = $h;
                $camera_config->fixed[$i]->m = $m;
        }
        
        if (!save_config($tmp)) {
                echo $config_error . "<br>";
                return NULL;
        }

        return $tmp;
}


function call_low_level_ssh_update()
{
        return call_update_daemon("http://127.0.0.1:10080/update/ssh");
}

function update_ssh()
{
        global $config_error, $msg, $ssh_file, $system_output;

        $tmp = load_config();
        if ($tmp === FALSE) {
                $msg .= $config_error;
                return NULL;
        }

        $ssh_config = $tmp->ssh;
        if (!$ssh_config) {
                $ssh_config = new StdClass();
                $tmp->ssh = $ssh_config;
                if (!save_config($tmp)) {
                        echo $config_error . "<br>";
                        return NULL;
                }
        }

        $key1 = $_REQUEST['key1'];
        // FIXME: regexp for SSH key...?
        $ssh_config->key1 = $key1;

        $key2 = $_REQUEST['key2'];
        // FIXME: regexp for SSH key...?
        $ssh_config->key2 = $key2;

        $key3 = $_REQUEST['key3'];
        // FIXME: regexp for SSH key...?
        $ssh_config->key3 = $key3;

        if (!save_config($tmp)) {
                echo $config_error . "<br>";
                return NULL;
        }

        $contents = "";
        if ($ssh_config->key1 != "") 
                $contents .= $ssh_config->key1 . "\n";
        if ($ssh_config->key2 != "") 
                $contents .= $ssh_config->key2 . "\n";
        if ($ssh_config->key3 != "") 
                $contents .= $ssh_config->key3 . "\n";
        
        $err = file_put_contents($ssh_file, $contents);
        if ($err === FALSE) {
                $msg .= "Failed to save the SSH configuration file. Call for help!";
                return FALSE;
        }

        $output = call_low_level_ssh_update(); 
        if ($output == FALSE) 
                return FALSE;
        $system_output = $output;
        
        return $tmp;
}

function update_opensensordata()
{
        global $config_error, $msg, $ssh_file, $system_output, $osd_dir;

        $tmp = load_config();
        if ($tmp === FALSE) {
                $msg .= $config_error;
                return NULL;
        }

        $osd_config = $tmp->opensensordata;
        if (!$osd_config) {
                $osd_config = new StdClass();
                $tmp->ssh = $osd_config;
                if (!save_config($tmp)) {
                        echo $config_error . "<br>";
                        return NULL;
                }
        }

        /*
        $server = $_REQUEST['server'];
        if (!filter_var($server, FILTER_VALIDATE_URL)) {
                $msg .= "Please verify the OpenSensorData.net server URL.";
                return NULL;
        }
        $osd_config->server = $server;
        */
        $osd_config->server = "http://opensensordata.net";

        $key = $_REQUEST['key'];
        if (!preg_match('/^[a-zA-Z0-9]{8,8}-[a-zA-Z0-9]{4,4}-[a-zA-Z0-9]{4,4}-[a-zA-Z0-9]{4,4}-[a-zA-Z0-9]{12,12}$/', $key)) {
                $msg .= "Please verify the OpenSensorData.net key.";
                return NULL;
        }
        $osd_config->key = $key;

        if (!save_config($tmp)) {
                echo $config_error . "<br>";
                return NULL;
        }
        
        return $tmp;
}

function update_sensors()
{
        global $config_error, $msg, $system_output;

        $tmp = load_config();
        if ($tmp === FALSE) {
                $msg .= $config_error;
                return NULL;
        }

        $sensors_config = $tmp->sensors;
        if (!$sensors_config) {
                $sensors_config = new StdClass();
                $tmp->sensors = $sensors_config;
                if (!save_config($tmp)) {
                        echo $config_error . "<br>";
                        return NULL;
                }
        }

        $sensors_config->rht03_1 = "no";
        if (isset($_REQUEST['rht03_1']) && !isset($_REQUEST['trh'])) {
                $trh = $_REQUEST['rht03_1'];
                $sensors_config->trh = ($trh == "yes")? "yes" : "no";
        } 
        if (isset($_REQUEST['trh'])) {
                $trh = $_REQUEST['trh'];
                $sensors_config->trh = ($trh == "yes")? "yes" : "no";
        } 

        if (isset($_REQUEST['rht03_2']) && !isset($_REQUEST['trhx'])) {
                $trhx = $_REQUEST['rht03_2'];
                $sensors_config->trhx = ($trhx == "yes")? "yes" : "no";
        } 
        if (isset($_REQUEST['trhx'])) {
                $trhx = $_REQUEST['trhx'];
                $sensors_config->trhx = ($trhx == "yes")? "yes" : "no";
        } 

        $sensors_config->light = "no";
        if (isset($_REQUEST['light'])) {
                $light = $_REQUEST['light'];
                $sensors_config->light = ($light == "yes")? "yes" : "no";
        }

        $sensors_config->usbbat = "no";
        if (isset($_REQUEST['usbbat'])) {
                $usbbat = $_REQUEST['usbbat'];
                $sensors_config->usbbat = ($usbbat == "yes")? "yes" : "no";
        }

        $update = $_REQUEST['update'];
        if (($update != "5") && ($update != "10") && ($update != "15") 
            && ($update != "30") && ($update != "60") && ($update != "120") 
            && ($update != "1")) {
                $msg .= "Invalid update period.";
                return NULL;
        }
        $sensors_config->update = $update;

        $upload = $_REQUEST['upload'];
        if (($upload != "fixed") && ($upload != "periodical")) {
                $msg .= "Invalid value for the upload. Only 'fixed' or 'periodical' are allowed.";
                return NULL;
        }
        $sensors_config->upload = $upload;

        $period = $_REQUEST['period'];
        if (($period == "") || !is_numeric($period) || ($period < 1) || ($period > 60)) {
                $msg .= "Invalid value for the period. It should be a positive number between 1 and 60 minutes.";
                return NULL;
        }
        $sensors_config->period = $period;

        for ($i = 0; $i < 4; $i++) {
                $hi = "h" . $i;
                $mi = "m" . $i;

                $h = $_REQUEST[$hi];
                if (($h != "") && (!is_numeric($h) || ($h < 0) || ($h > 23))) {
                        $msg .= "Invalid hour for the fixed upload times ('" . $h . "').";
                        return NULL;
                }
                $m = $_REQUEST[$mi];
                if (($m != "") && (!is_numeric($m) || ($m < 0) || ($m > 59))) {
                        $msg .= "Invalid minute for the fixed upload times ('" . $m . "').";
                        return NULL;
                }
                $sensors_config->fixed[$i]->h = $h;
                $sensors_config->fixed[$i]->m = $m;
        }

        if (!save_config($tmp)) {
                echo $config_error . "<br>";
                return NULL;
        }
        
        return $tmp;
}

function update_power()
{
        global $config_error, $msg, $system_output;

        $tmp = load_config();
        if ($tmp === FALSE) {
                $msg .= $config_error;
                return NULL;
        }

        $power_config = $tmp->power;
        if (!$power_config) {
                $power_config = new StdClass();
                $tmp->power = $power_config;
                if (!save_config($tmp)) {
                        echo $config_error . "<br>";
                        return NULL;
                }
        }

        $poweroff = $_REQUEST['poweroff'];
        $power_config->poweroff = ($poweroff == "yes")? "yes" : "no";

        if (!save_config($tmp)) {
                echo $config_error . "<br>";
                return NULL;
        }
        
        return $tmp;
}

$op = isset($_REQUEST['op'])? $_REQUEST['op'] : NULL;
//echo "section=" . $section . "<br>\n";
if ($op == "update") {

        $section = $_REQUEST['section'];

        if ($section == "general") 
                $new_config = update_general();
        else if ($section == "login") 
                $new_config = update_login();
        else if ($section == "wifi") 
                $new_config = update_wifi();
        else if ($section == "wired") 
                $new_config = update_wired();
        else if ($section == "gsm") 
                $new_config = update_gsm();
        else if ($section == "camera") 
                $new_config = update_camera();
        else if ($section == "ssh") 
                $new_config = update_ssh();
        else if ($section == "opensensordata") 
                $new_config = update_opensensordata();
        else if ($section == "sensors") 
                $new_config = update_sensors();
        else if ($section == "power") 
                $new_config = update_power();

        if ($new_config != NULL) {
                $config = $new_config;
        }
}

?>
