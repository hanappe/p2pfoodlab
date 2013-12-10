<?php /*  

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

?>

<?php
   for ($i = 0; $i < count($this->accounts); $i++) {
       $account = $this->accounts[$i];
       $url = $base_url . "/people/" . $account->username;
echo "<div class=\"strip\">\n";
echo "  <div class=\"content_box frame margin\">\n";
echo "    <a href=\"$url\" class=\"rightmargin\">" . $account->username . "</a>\n";
echo "  </div>\n";
echo "</div>\n";
   }
?>       
