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

class Doc
{
        public $id;
        public $account;
        public $path;
        public $type;

        public function import($row) 
        {
                $this->id = $row['id'];
                $this->account = $row['account'];
                $this->path = $row['path'];
                $this->type = $row['type'];
        }
        
        public function store() 
        {
                global $mysqli;
                
                if (isset($this->id)) 
                        return false;

                $query = ("INSERT INTO docs VALUES ("
                          . "NULL,"
                          . $mysqli->real_escape_string($this->account) . ","
                          . "'" . $mysqli->real_escape_string($this->path) . "',"
                          . "'" . $mysqli->real_escape_string($this->type) . "')");

                echo $query . "<br/>\n";

                $res = $mysqli->query($query);
                if ($mysqli->errno) {
                        //echo $mysqli->error . "<br/>\n";
                        return false;
                }

                $this->id = $mysqli->insert_id;

                return true;
        }

        public static function loadall($account) 
        {
                global $mysqli;

                $docs = array();
                $query = "SELECT * FROM docs WHERE account=" . $account;
                $res = $mysqli->query($query);
                if (!$mysqli->errno) {
                        while ($row = $res->fetch_assoc()) {
                                $d = new Doc();
                                $d->import($row);
                                $docs[] = $d;
                        }
                }
                return $docs;
        }
}

?>
