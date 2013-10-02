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
          <a class="pagemenu" href="index.php?op=logout">Logout</a> 
         </div>
      </div>
      
      <div class="main">
        TODO
<?php
/*

- status network interfaces
- status disk usage
- latest successful update

*/
?>
      </div>
    </div>
  </body>
</html>
