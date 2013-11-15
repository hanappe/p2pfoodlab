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

  /*
FIXME: session_start();

if (isset($_SESSION['connected'])) {
        if (!isset($_SESSION['timeout'])) {
                header("HTTP/1.1 401 Unauthorized: session timeout");
                exit(0);
        } else if ($_SESSION['timeout'] + 3600 < time()) {
                header("HTTP/1.1 401 Unauthorized: session timeout");
                exit(0);
        }        
} else {
        header("HTTP/1.1 401 Unauthorized: no session");
        exit(0);
}
  */

echo file_get_contents("http://127.0.0.1:10080/test/camera"); 

?>
