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
require_once "error.inc.php";

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
      
      <div class="main"> 

      <p>The status page is still being developed.</p> 

      <p>Please use:

      <ul> 

      <li>the <b><a href="configuration.php">configuration page</a></b> to change the settings of the
      sensorbox,</li>

      <li>the <b><a href="updates.php">update page</a></b> to install the latest
      software, and</li>

      <li>the <b><a href="log.php">log page</a></b> to verify the output of the low-level tools.</li>

      </ul></p>

      </div>
    </div>
  </body>
</html>
