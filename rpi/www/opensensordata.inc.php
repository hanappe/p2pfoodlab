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

class OpenSensorData 
{
        public $msg = "";
        public $server = "http://opensensordata.net";
        public $key = "no-key";
        public $cachedir = FALSE;

        function __construct($server, $key, $cachedir) 
        {
                $this->server = $server;
                $this->key = $key;
                $this->cachedir = $cachedir;
        }

        public function decode($json)
        {
                $r = json_decode($json);

                if ($r === NULL) {
                        $json_error = "";
                        switch (json_last_error()) {
                        case JSON_ERROR_NONE:
                                $json_error = 'No errors';
                                break;
                        case JSON_ERROR_DEPTH:
                                $json_error = 'Maximum stack depth exceeded';
                                break;
                        case JSON_ERROR_STATE_MISMATCH:
                                $json_error = 'Underflow or the modes mismatch';
                                break;
                        case JSON_ERROR_CTRL_CHAR:
                                $json_error = 'Unexpected control character found';
                                break;
                        case JSON_ERROR_SYNTAX:
                                $json_error = 'Syntax error, malformed JSON';
                                break;
                        case JSON_ERROR_UTF8:
                                $json_error = 'Malformed UTF-8 characters, possibly incorrectly encoded';
                                break;
                        default:
                                $json_error = 'Unknown error';
                                break;
                        }
                        $this->msg .= "Failed to parse the data: " . $json_error;
                        return NULL;
                }

                return $r;
        }

        public function get($path) 
        {
                $options = array('http' => 
                                 array ("header" => ("Content-type: application/json\r\n"
                                                     . "X-OpenSensorData-Key: " . $this->key . "\r\n")
                                        )
                                 );

                $context  = stream_context_create($options);
                $url = $this->server . "/" . $path;
                $r = file_get_contents($url, false, $context);

                /* echo "-------------------------------------------<br>\n"; */
                /* echo "GET $url<br>\n"; */
                /* echo "-> "; var_dump($r); echo "<br>\n"; */

                if ($r === FALSE) 
                        return FALSE;

                return $this->decode($r);
        }

        public function put($path, $data) 
        {
                $options = array('http' => 
                                 array (
                                        "method" => "PUT",
                                        "header" => ("Content-type: application/json\r\n"
                                                     . "X-OpenSensorData-Key: " . $this->key . "\r\n"),
                                        "content" => json_encode($data)
                                        )
                                 );

                $context  = stream_context_create($options);
                $url = $this->server . "/" . $path;
                $r = file_get_contents($url, false, $context);

                /* echo "-------------------------------------------<br>\n"; */
                /* echo "PUT $url<br>\n"; */
                /* echo "-> "; var_dump($data); echo "<br>\n"; */
                /* echo "-> "; var_dump($r); echo "<br>\n"; */

                if ($r === FALSE) 
                        return FALSE;
                
                return $this->decode($r);
        }


        public function get_my_account()
        {
                return $this->get("account.json");
        }

        public function get_account($name)
        {
                return $this->get("accounts/" . $name . ".json");
        }

        public function get_datastream_by_name($account, $group, $name)
        {
                return $this->get("datastreams/" . $account->name . "/" . $group . "/" . $name . ".json");
        }

        public function get_datastream_by_id($id)
        {
                return $this->get("datastreams/" . $id . ".json");
        }

        public function create_datastream($name, $description, $unit, $cached_ok)
        {
                if ($cached_ok) {
                        $filename = $this->cachedir . "/" . $name . ".json";
                        $json = file_get_contents($filename);
                        if ($json != FALSE) {
                                // FIXME: if the description or the unit changed, shouldn't we create a new definition?
                                return $this->decode($json);
                        }
                }
                
                $d = new StdClass();
                $d->name = $name;
                $d->description = $description;
                $d->unit = $unit;
                
                $reply = $this->put("datastreams/", $d);
                if ($reply === FALSE) {
                        $this->msg = " Failed to post the datastream definition to the OpenSensorData server. Call for help!";
                        return FALSE;
                }

                if (1) {
                        $filename = $this->cachedir . "/" . $name . ".json";
                        $err = file_put_contents($filename, json_encode($reply));
                        if ($err === FALSE) {
                                $this->msg = " Failed to save the OpenSensorData.net datastream definition. Call for help!";
                                return FALSE;
                        }
                }

                return $reply;
        }
}

?>
