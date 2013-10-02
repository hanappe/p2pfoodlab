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

$homedir = "/var/p2pfoodlab";
$config_file = $homedir . "/etc/config.json";
$interfaces_file = $homedir . "/etc/interfaces";
$dhcpd_file = $homedir . "/etc/dhcpd.conf";
$wvdial_file = $homedir . "/etc/wvdial.conf";
$ssh_file = $homedir . "/etc/authorized_keys";
$crontab_file = $homedir . "/etc/crontab";
$osd_dir = $homedir . "/etc/opensensordata";

$config_error = "";
$config_version_major = 1;
$config_version_minor = 0;
$config_version_patch = 0;

function get_random_string($length)
{
        $valid_chars = "abcdefghijklmnopqrstuvwABCDEFGHIJKLMNOPQRSTUVW0123456789@:.?!%&*-_=+<>#~";

        $random_string = "";
        $num_valid_chars = strlen($valid_chars);
        
        for ($i = 0; $i < $length; $i++) {
                $random_pick = mt_rand(1, $num_valid_chars);
                $random_char = $valid_chars[$random_pick-1];
                $random_string .= $random_char;
        }

        return $random_string;
}

function load_config()
{
        global $config_error, $config_file;

        $config_error = "";

        $json = file_get_contents($config_file);
        if ($json === FALSE) {
                $config_error = "Failed to load the configuration file. Call for help!";
                return FALSE;
        }
        $config = json_decode($json);
        //echo "json_decode=" . var_dump($config) . "<br>";
        
        if ($config === NULL) {
                switch (json_last_error()) {
                case JSON_ERROR_NONE:
                        $msg = ' - No errors';
                        break;
                case JSON_ERROR_DEPTH:
                        $msg = ' - Maximum stack depth exceeded';
                        break;
                case JSON_ERROR_STATE_MISMATCH:
                        $msg = ' - Underflow or the modes mismatch';
                        break;
                case JSON_ERROR_CTRL_CHAR:
                        $msg = ' - Unexpected control character found';
                        break;
                case JSON_ERROR_SYNTAX:
                        $msg = ' - Syntax error, malformed JSON';
                        break;
                case JSON_ERROR_UTF8:
                        $msg = ' - Malformed UTF-8 characters, possibly incorrectly encoded';
                        break;
                default:
                        $msg = ' - Unknown error';
                        break;
                }
                $config_error = "Failed to parse the configuration file. Call for help! Error" . $msg;
                return FALSE;
        }

        return $config;
}

function save_config($config)
{
        global $config_error, $config_file;

        $config_error = "";

        $json = json_encode($config);
        if (!$json) {
           $config_error = "Failed to encode the configuration file. Call for help!";
           return FALSE;
        }

        $err = file_put_contents($config_file, $json);
        if ($err === FALSE) {
           $config_error = "Failed to save the configuration file. Call for help!";
           return FALSE;
        }

        return TRUE;
}

$config = load_config();
if ($config === FALSE) {
        echo $config_error . "<br>";
        exit();
}

?>
