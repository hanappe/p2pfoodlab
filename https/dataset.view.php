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


if (1) {
        $osd = new OpenSensorData(null, null, null);
        $start_timestamp = date("U");
        $end_timestamp = 0;
        
        $datastreams = $this->group->datastreams;
        for ($j = 0; $j < count($datastreams); $j++) {
                $datastream = $datastreams[$j];
                if (isset($this->datastreams[$datastream->id])) {
                        $interval = $osd->get_datastream_interval($datastream->id);
                        if ($interval && (count($interval) >= 2)) {
                                $s = strtotime($interval[0]);
                                $e = strtotime($interval[1]);
                                //echo "datastream " . $datastream->id . ": start " . date("Y-m-d H:i:s", $s) . ", end " . date("Y-m-d H:i:s", $e) . "<br>\n";
                                if ($s < $start_timestamp)
                                        $start_timestamp = $s;
                                if ($e > $end_timestamp)
                                        $end_timestamp = $e;
                        }
                }
        }

        $start_year = date("Y", $start_timestamp);
        $start_month = date("m", $start_timestamp);
        $start_day = date("d", $start_timestamp);

        $end_year = date("Y", $end_timestamp);
        $end_month = date("m", $end_timestamp);
        $end_day = date("d", $end_timestamp);

 } else {

        $start_year = date("Y");
        $start_month = date("m");
        $start_day = 1;

        $end_year = $start_year;
        $end_month = $start_month;
        $end_day = date("d");

 }

$start_date = sprintf("%04d-%02d-%02d", $start_year, $start_month, $start_day);
$start_date_js = sprintf('%04d,%02d,%02d', $start_year, ($start_month-1), $start_day);

$end_date = sprintf("%04d-%02d-%02d", $end_year, $end_month, $end_day);
$end_date_js = sprintf('%04d,%02d,%02d', $end_year, ($end_month-1), $end_day);

$range = sprintf("%04d%02d%02d/%04d%02d%02d", 
                 $start_year, $start_month, $start_day, 
                 $end_year, $end_month, $end_day);

?>
<script type="text/javascript"> 
var startDate = new Date(<?php echo $start_date_js ?>);
var endDate = new Date(<?php echo $end_date_js ?>);

<?php
echo "var graphs = []\n";
echo "var slideshows = []\n";
echo "var blockRedraw = false;\n";
echo "var datastreams = [ ";
$n = 0;
$datastreams = $this->group->datastreams;
for ($j = 0; $j < count($datastreams); $j++) {
        $datastream = $datastreams[$j];
        if (isset($this->datastreams[$datastream->id])) {
                if ($n > 0) echo ", ";
                echo $datastream->id;
                $n++;
        }
}
echo " ];\n";

$photostreams = $this->group->photostreams;
echo "var photostreams = [ ";
$n = 0;
for ($j = 0; $j < count($photostreams); $j++) {
        $photostream = $photostreams[$j];
        if ($n > 0) echo ", ";
        echo $photostream->id;
        $n++;
}
echo " ];\n";

?>

var timer = null;
var updating = false;
var osd = new OpenSensorData("<?php echo $base_url ?>/opensensordata", null);

function twoDigits(n)
{
        if (n < 10) return "0" + n;
        else return "" + n;
}

function formatDate(d)
{
        return ("" + d.getFullYear()
                + twoDigits(1 + d.getMonth())
                + twoDigits(d.getDate()));
}

function formatDate2(d)
{
        return ("" + d.getFullYear()
                + "-" + twoDigits(1 + d.getMonth())
                + "-" + twoDigits(d.getDate()));
}

function checkUpdating()
{
        if (!updating) return;
        for (i = 0; i < datastreams.length; i++) 
                if (graphs[i].updating) return;
        for (i = 0; i < photostreams.length; i++)
                if (slideshows[i].updating) return;
        updating = false;
        for (i = 0; i < datastreams.length; i++) {
                if (graphs[i].numRows() > 0)
                        graphs[i].resetZoom();
        }
}

function updateDatasets()
{
        var range = formatDate(startDate) + "/" + formatDate(endDate);
        //var d = formatDate(endDate);

        if (updating) return;

        updating = true;

        for (i = 0; i < datastreams.length; i++) {
                var path = ("filter/" + datastreams[i] + "/" + range + ".json");
                //var path = ("datapoints/" + datastreams[i] + "/" + range + ".json");
                //var path = ("filter/" + datastreams[i] + "/" + d + ".json");
                var updater = { 
                        "graph": graphs[i],
                        "callback": function(me, data) { 
                                //alert(data);
                                me.graph.updateOptions({ "file": data, "customBars": true, "errorBars": true }); 
                                me.graph.updating = false;
                                checkUpdating();
                        }
                };
                graphs[i].updating = true;
                osd.getdata(path, updater, updater.callback);
        }
        for (i = 0; i < photostreams.length; i++) {
                var path = ("photos/" + photostreams[i] + "/" + range + ".json");
                //var path = ("photos/" + photostreams[i] + "/" + d + ".json");
                var updater = { 
                        "slideshow": slideshows[i],
                        "callback": function(me, data) { 
                                me.slideshow.setPhotos(data); 
                                me.slideshow.updating = false;
                                checkUpdating(); 
                        }
                };
                slideshows[i].updating = true;
                osd.get(path, updater, updater.callback);
        }
}

function changeStartDate(calendar, date)
{
        var div = document.getElementById("startDate");
        div.innerHTML = formatDate2(calendar.date);
        startDate = calendar.date;
        if (timer != null) {
                clearTimeout(timer); 
                timer = null;
        }
        timer = setTimeout(updateDatasets, 2000);
}

function changeEndDate(calendar, date)
{
        var div = document.getElementById("endDate");
        div.innerHTML = formatDate2(calendar.date);
        endDate = calendar.date;
        if (timer != null) {
                clearTimeout(timer); 
                timer = null;
        }
        timer = setTimeout(updateDatasets, 2000);
}
</script>

<?php

$this->host->load_sensorboxes();

if ($this->errmsg) {
?>
      <div class="strip">
        <div class="content_box frame">
          <div class="margin">
          <?php echo $this->errmsg; ?>
          </div>
        </div>
      </div>

<?php
} else if (($this->visitor != NULL)
           && ($this->host != NULL)
           && ($this->visitor->id == $this->host->id)
           && (count($this->host->sensorbox) <= 0)) {

        
        echo "    COUNT " . count($host->sensorbox) . "\n";
        
        echo "    <div class=\"strip\">\n";
        echo "        <div class=\"content_box frame margin\">\n";
        echo "             " . _s("For more information about this section, check out the help section") . " (<a href=\"" . url_s('help#sensorbox') . "\">" . _s("here") . "</a>).\n";
        echo "        </div>\n";
        echo "    </div>\n";

 } else {
?>

      <div class="strip">
        <div class="content_box frame margin">
          <?php 
             echo $this->group->name;
             if (isset($this->group->description) 
                 && (strlen($this->group->description) > 0)) {
                 echo ": " . $this->group->description;
             }
          ?>
        </div>
      </div>

      <div class="strip">
        <div class="content_box frame">
          <div class="datepicker margin">

<? /*
            <?php _p("show data of") ?>
            <a href="javascript:void(0);" id="endDate" class="date"><?php _p("today") ?></a>.
*/ ?>

            <?php _p("show data from") ?>
            <a href="javascript:void(0);" id="startDate" class="date"><?php echo $start_date ?></a>
            <?php _p("to") ?>
            <a href="javascript:void(0);" id="endDate" class="date"><?php echo $end_date ?></a>.
            <script type="text/javascript">
            Calendar.setup(
              {
                dateField: 'startDate',
                triggerElement: 'startDate',
                selectHandler: changeStartDate
              }
            );
<? /**/ ?>
            Calendar.setup(
              {
                dateField: 'endDate',
                triggerElement: 'endDate',
                selectHandler: changeEndDate
              }
            );
            </script>
          </div>
        </div>
      </div>

<?php
        /* if ($this->group->app != $osd_appid) */
        /*         continue; */
                
        for ($j = 0; $j < count($photostreams); $j++) {
                $photostream = $photostreams[$j];
                $id = 'photostream_' . $photostream->id;
                $path = "photos/" . $photostream->id . "/$range.json";
?>
                <div class="strip">
                  <div class="content_box frame centered">
                    <div id="<?php echo $id ?>" class="photostreamviewer"></div>
                    <script type="text/javascript"> 
                    var slideshow = new Photostream("<?php echo $id ?>", [], 
                                                    "http://opensensordata.net", ".jpg", 
                                                    575, 510);
                    slideshows.push(slideshow);
                    </script>
                  </div>
                </div>
<?php 
        }

        for ($j = 0; $j < count($datastreams); $j++) {
                $datastream = $datastreams[$j];
                if (isset($this->datastreams[$datastream->id])) {
                        $id = 'datastream_' . $datastream->id;
?>
                        <div class="strip">
                          <div class="content_box frame centered datastreamviewer">
                            <?php echo "$datastream->name\n" ?>
                            <div id="<?php echo $id ?>" class="datastream margin"></div>
                            <script type="text/javascript">
                            var graph = new Dygraph(document.getElementById("<?php echo $id ?>"),
                                                    [], 
                                                  { labels: [ "Date", "<?php echo $datastream->unit ?>" ],
                                                    includeZero: true, 
                                                    showRangeSelector: false,
                                                    customBars: true,
                                                    errorBars: true,
                                                    drawCallback: function(me, initial) {
                                                        if (blockRedraw || initial) return;
                                                        blockRedraw = true;
                                                        var range = me.xAxisRange();
                                                        for (var j = 0; j < graphs.length; j++) {
                                                            if (graphs[j] == me) continue;
                                                            graphs[j].updateOptions( { dateWindow: range } );
                                                        }
                                                        blockRedraw = false;
                                                    }
                                                  } );
                            graphs.push(graph); 
                            </script>
                          </div>
                        </div>
<?php 
              }
        }
 }
?>

<script type="text/javascript"> 
updateDatasets();
</script>
