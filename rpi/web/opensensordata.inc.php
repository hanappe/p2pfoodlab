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
        public $appkey = "no-key";
        public $err = false;

        function __construct($server, $key, $cachedir) 
        {
                if ($server) $this->server = $server;
                if ($key) $this->key = $key;
                if (isset($cachedir))
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
                                                     . "X-OpenSensorData-Key: " . $this->key . "\r\n"),
                                        "ignore_errors" => "1"
                                        )
                                 );

                $context  = stream_context_create($options);
                $url = $this->server . "/" . $path;
                $r = file_get_contents($url, false, $context);

                /* echo "-------------------------------------------<br>\n"; */
                /* echo "GET $url<br>\n"; */
                /* echo "-> "; var_dump($r); echo "<br>\n"; */

                if ($r === FALSE) {
                        $this->msg = "Failed to retrieve data from OpenSensorData.net: unknown error. ";
                        return FALSE;
                }

                $json = $this->decode($r);
                if (isset($json->error)) {
                        $this->msg = $json->msg;
                        return false;
                }

                return $json;
        }

        public function put($path, $data) 
        {
                $options = array('http' => 
                                 array (
                                        "method" => "PUT",
                                        "header" => ("Content-type: application/json\r\n"
                                                     . "X-OpenSensorData-Key: " . $this->key . "\r\n"),
                                        "content" => json_encode($data),
                                        "ignore_errors" => "1"
                                        )
                                 );

                $context  = stream_context_create($options);
                $url = $this->server . "/" . $path;
                $r = file_get_contents($url, false, $context);

                /* echo "-------------------------------------------<br>\n"; */
                /* echo "PUT $url<br>\n"; */
                /* echo "-> "; var_dump($data); echo "<br>\n"; */
                /* echo "-> "; var_dump($r); echo "<br>\n"; */

                if ($r === FALSE) {
                        $this->msg = "Failed to retrieve data from OpenSensorData.net: unknown error. ";
                        return FALSE;
                }

                $json = $this->decode($r);
                if (isset($json->error)) {
                        $this->msg = $json->msg;
                        return false;
                }

                return $json;
        }

        public function put_with_appkey($path, $data) 
        {
                $options = array('http' => 
                                 array (
                                        "method" => "PUT",
                                        "header" => ("Content-type: application/json\r\n"
                                                     . "X-OpenSensorData-AppKey: " . $this->appkey . "\r\n"),
                                        "content" => json_encode($data),
                                        "ignore_errors" => "1"
                                        )
                                 );

                $context  = stream_context_create($options);
                $url = $this->server . "/" . $path;
                $r = file_get_contents($url, false, $context);

                /* echo "-------------------------------------------<br>\n"; */
                /* echo "PUT/AppKey $url<br>\n"; */
                /* echo "-> data: "; var_dump($data); echo "<br>\n"; */
                /* echo "-> json: " . json_encode($data) . "<br>\n"; */
                /* echo "<- resp header: "; var_dump($http_response_header); echo "<br>\n"; */
                /* echo "<- resp data: "; var_dump($r); echo "<br>\n"; */

                if ($r === FALSE) {
                        $this->msg = "Failed to retrieve data from OpenSensorData.net: unknown error. ";
                        return FALSE;
                }

                $json = $this->decode($r);
                if (isset($json->error)) {
                        $this->msg = $json->msg;
                        return false;
                }

                return $json;
        }

        public function get_my_account()
        {
                return $this->get("account.json");
        }

        public function get_account($name) 
        {
                return $this->get("accounts/" . $name . ".json");
        }

        public function get_group($id) {
                return $this->get("groups/" . $id . ".json");
        }

        public function create_account($name, $email) 
        {
                $d = new StdClass();
                $d->username = $name;
                $d->email = $email;

                $account = $this->put_with_appkey("accounts/", $d);
                var_dump($account);

                return $account;
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
                if ($cached_ok && $this->cachedir) {
                        $filename = $this->cachedir . "/" . $name . ".json";
			if (is_readable($filename)) {
			  $json = file_get_contents($filename);
			  if ($json != FALSE) {
			    // FIXME: if the description or the unit changed, shouldn't we create a new definition?
			    return $this->decode($json);
			  }
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

                if ($this->cachedir) {

		echo "trying to store datastream in $filename<br>\n";

                        $filename = $this->cachedir . "/" . $name . ".json";
                        $err = file_put_contents($filename, json_encode($reply));
                        if ($err === FALSE) {
                                $this->msg = " Failed to save the OpenSensorData.net datastream definition to $filename. ";
                                return FALSE;
                        }
                }

		echo "datastream stored in $filename<br>\n";
		
                return $reply;
        }
}

?>
