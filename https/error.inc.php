<?php
/*  
    P2P Food Lab 

    P2P Food Lab is an open, collaborative platform to develop an
    alternative food value system,.

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

require_once "account.model.php";
require_once "page.model.php";

// downForMaintenanceUntil("Sat, 8 Oct 2011 18:27:00 GMT")
function downForMaintenanceUntil($date)
{
        header('HTTP/1.1 503 Service Temporarily Unavailable');
        header('Retry-After: $date');
        exit(0);
}

function error($errmsg)
{
        global $visitor_account;
        $page = new ErrorPage($visitor_account, $errmsg);
        $page->generate();
        exit();
}

function internalServerError($details)
{
        error("An internal server error occured.<br>(Details: " . $details . ")");
}

function badRequest($details)
{
        error("Bad request.<br>(Details: " . $details . ")");
}

function unauthorized($msg)
{
        error("Unauthorized access: " . $msg . "<br>Please log in again in case your sessions expired.");
}

function jsonSuccess()
{
        $response = new StdClass();
        $response->success = true;
        header('Content-type: application/json');
        echo json_encode($response) . "\n";
        exit();
}

function jsonError($errmsg)
{
        $response = new StdClass();
        $response->success = false;
        $response->message = $errmsg;
        header('Content-type: application/json');
        echo json_encode($response) . "\n";
        exit();
}

function jsonInternalServerError($details)
{
        jsonError("An internal server error occurred: " . $details);
}

function jsonBadRequest($details)
{
        jsonError("Bad request: " . $details);
}

function jsonUnauthorized($msg)
{
        jsonError("Unauthorized: " . $msg);
}

?>
