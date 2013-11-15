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

require_once "config.inc.php";

session_start();

function show_login()
{
        global $config;


        $config->login->pwsalt = get_random_string(12);

 //echo "show_login op = ". $op . "<br>\n";

        if (!save_config($config)) {
                echo $config_error . "<br>";
                exit();
        }

        unset($_SESSION['connected']);
        unset($_SESSION['timeout']);
        require_once "login.inc.php";
        exit();
}

$op = isset($_REQUEST['op'])? $_REQUEST['op'] : NULL;

 //echo "op = ". $op . "<br>\n";

if ($op == "logout") {
        show_login();

} else if ($op == "login") {

        $remote_hash = $_REQUEST['pwhash'];

        //echo "remote_hash = ". $remote_hash . "<br>\n";
        if (!$remote_hash) {
                show_login();
        }

        $local_hash = md5($config->login->pwsalt . $config->login->pwhash);
        //echo "pwsalt = ". $config->login->pwsalt . "<br>\n";
        //echo "pwhash = ". $config->login->pwhash . "<br>\n";
        //echo "local_hash = ". $local_hash . "<br>\n";

        if ($local_hash != $remote_hash) 
                show_login();

        $_SESSION['connected'] = true;
        $_SESSION['timeout'] = time();


} else if ($op == "poweroff") {

        $output = file_get_contents("http://127.0.0.1:10080/update/poweroff");


} else if (isset($_SESSION['connected'])) {
        if (!isset($_SESSION['timeout'])) {
                show_login();
        } else if ($_SESSION['timeout'] + 3600 < time()) {
                show_login();
        } else {
                $_SESSION['timeout'] = time();
        }        

} else {
        show_login();
}

?>
