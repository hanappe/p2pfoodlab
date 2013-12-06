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

$osd_server = "http://opensensordata.net";
$osd_appid = 0;
$osd_appkey = "xxx";

$base_url = "https://www.example.com";
$contact_address = "me@example.com";

$docsdir = "xxx";

$dbhost = "localhost";
$database = "xxx";
$dbuser = "xxx";
$dbpass = "xxx";

function url($path) 
{
        global $base_url;
        echo $base_url . "/" . $path;
}

function url_s($path) 
{
        global $base_url;
        return $base_url . "/" . $path;
}

?>
