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

require_once "config.inc.php";
require_once "locale.model.php";
require_once "db.inc.php";
require_once "input.inc.php";
require_once "error.inc.php";
require_once "account.model.php";
require_once "post.model.php";
require_once "tag.model.php";
require_once "channel.model.php";
require_once "doc.model.php";

session_start();
db_connect();

$visitor_account = load_visitor_account();
if ($visitor_account == NULL)
        unauthorized("Did your login expire?");

$url = input_get_url();

$op = input_get('op');
if ($op != "postdoc") 
        badRequest("Invalid operation");

for ($i = 0; $i < count($_FILES['docs']['name']); $i++) {

        switch ($_FILES['docs']['error'][$i]) {
        case UPLOAD_ERR_OK: 
                break;
        case UPLOAD_ERR_INI_SIZE:
        case UPLOAD_ERR_FORM_SIZE:
                badRequest("Image size too big");
                break;
        case UPLOAD_ERR_PARTIAL:
                badRequest("Upload incomplete");
                break;
        case UPLOAD_ERR_NO_FILE:
                badRequest("File has no data");
                break;
        default:
                badRequest("Upload failed (err " . $_FILES['docs']['error'][$i] . ")");
                break;
        }

        if (!is_uploaded_file($_FILES['docs']['tmp_name'][$i])) {
                badRequest("Upload failed");
        }

        if ($_FILES['docs']['size'][$i] > 10000000) {
                badRequest("Image size too big");
        }

        $tmpname = $_FILES['docs']['tmp_name'][$i];

        //echo "tmpname = $tmpname<br>\n";

        $finfo = finfo_open(FILEINFO_MIME_TYPE);
        $mimetype = finfo_file($finfo, $tmpname);
                
        if ($mimetype != "image/jpeg") {
                continue;
        }
                
        $d1 = $visitor_account->id;

        $dir = sprintf("%s/%03d/upload", $docsdir, $d1);
        if (!is_dir($dir) && !mkdir($dir, 0775, true)) {
                internalServerError("Failed to create the upload dir.");
        }
                
        // Generate a sort of random extension...
        $r =  sprintf('%02x%02x%02x', mt_rand(0, 0xff), 
                      mt_rand(0, 0xff), mt_rand(0, 0xff));
                
        $path = sprintf("%03d/upload/%d-upload-%s",
                        $d1, $visitor_account->id, $r);

        $filename = sprintf("%s/%s.jpg", $docsdir, $path);        
        if (copy($tmpname, $filename) === FALSE) {
                internalServerError("Failed to copy the file.");
        }

        // Generate the different size version of the image
        $img = new Imagick($tmpname);
        $size = getimagesize($tmpname);

        try {

                $filename = sprintf("%s/%s-large.jpg", $docsdir, $path);        
                //echo "filename = $filename<br>\n";
                
                if (($size[0] > 1200) || ($size[1] > 1200)) {
                        $img->scaleImage(1200, 0); 
                        $d = $img->getImageGeometry();
                        if ($d['height'] > 1200)  $img->scaleImage(0, 1200); 
                        $img->writeImage($filename);
                } else {
                        if (!copy($tmpname, $filename)) 
                                internalServerError("Failed to store the photo.");
                }

                $filename = sprintf("%s/%s-medium.jpg", $docsdir, $path);        
                //echo "filename = $filename<br>\n";
                $img->scaleImage(800, 0); 
                $d = $img->getImageGeometry();
                if ($d['height'] > 800)  $img->scaleImage(0, 800); 
                $img->writeImage($filename);

                $filename = sprintf("%s/%s-small.jpg", $docsdir, $path);        
                //echo "filename = $filename<br>\n";
                $img->scaleImage(240, 0); 
                $d = $img->getImageGeometry();
                if ($d['height'] > 240)  $img->scaleImage(0, 240); 
                $img->writeImage($filename);
                $img->writeImage($filename);

                $filename = sprintf("%s/%s-tiny.jpg", $docsdir, $path);        
                //echo "filename = $filename<br>\n";
                $img->scaleImage(100, 0); 
                $d = $img->getImageGeometry();
                if ($d['height'] > 100)  $img->scaleImage(0, 100); 
                $img->writeImage($filename);
                $img->destroy();

                $img = new Imagick($tmpname);
                $filename = sprintf("%s/%s-thumb.jpg", $docsdir, $path);        
                $img->cropThumbnailImage(160, 160); 
                $img->writeImage($filename);
                $img->destroy();

        } catch (Exception $e) {
                internalServerError($e->getMessage());
        }

        $doc = new Doc();
        $doc->path = $path;
        $doc->account = $visitor_account->id;
        $doc->type = $mimetype;

        if (!$doc->store()) {
                internalServerError("Failed to store the photo in the database.");
        }

 }

header("Location: " . $url);

db_close();

?>
