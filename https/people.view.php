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

       $messages = $account->count_messages();

       $url = $base_url . "/people/" . $account->username;

 if (($messages <= 0) 
     && (count($account->sensorbox) <= 0)
     && (($this->visitor == NULL)
         || ($this->host == NULL)
         || ($this->visitor->id != $this->host->id))) {
         continue;
 }

echo "<div class=\"strip\">\n";
echo "  <div class=\"content_box frame margin\">\n";
echo "    <a href=\"$url\" class=\"rightmargin standout\">" . $account->username . "</a>\n";
 if ($messages == 1) {
         echo "     1 post\n";
 } else if ($messages > 1) {
         echo "     $messages posts\n";
 }
 if (count($account->sensorbox) == 1) {
         if ($messages > 0)          
                 echo "     - \n";
         echo "     <a href=\"$url/greenhouse/1\">1 dataset</a>\n";
 } else if (count($account->sensorbox) > 1) {
         if ($messages > 0)          
                 echo "     - \n";
         echo "     <a href=\"$url/greenhouse/1\">" . count($account->sensorbox) . " datasets</a>\n";
 }

echo "  </div>\n";
echo "</div>\n";
   }
?>       
