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


$today = date("Ymd");
$this_year = date("Y");
$this_month = date("m");
$start_date = sprintf("%04d-%02d-01", $this_year, $this_month);
$range = sprintf("%04d%02d01/%s", $this_year, $this_month, $today);

?>
<script type="text/javascript"> 
var startDate = new Date(<?php printf('%04d,%02d,%02d', $this_year, ($this_month-1), 1) ?>);
var endDate = new Date();

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
        //var range = formatDate(startDate) + "/" + formatDate(endDate);
        var d = formatDate(endDate);

        if (updating) return;

        updating = true;

        for (i = 0; i < datastreams.length; i++) {
                //var path = ("datapoints/" + datastreams[i] + "/" + range + ".json");
                var path = ("filter/" + datastreams[i] + "/" + d + ".json");
                var updater = { 
                        "graph": graphs[i],
                        "callback": function(me, data) { 
                                //alert(data);
                                me.graph.updateOptions({ file: data}); 
                                me.graph.updating = false;
                                checkUpdating();
                        }
                };
                graphs[i].updating = true;
                osd.getdata(path, updater, updater.callback);
        }
        for (i = 0; i < photostreams.length; i++) {
                //var path = ("photos/" + photostreams[i] + "/" + range + ".json");
                var path = ("photos/" + photostreams[i] + "/" + d + ".json");
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
           && (count($host->sensorbox) <= 0)) {
        
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
            <?php _p("show data of") ?>
            <a href="javascript:void(0);" id="endDate" class="date"><?php _p("today") ?></a>.

<? /*
            <?php _p("show data from") ?>
            <?php _p("show data from") ?>
            <a href="javascript:void(0);" id="startDate" class="date"><?php echo $start_date ?></a>
            <?php _p("to") ?>
            <a href="javascript:void(0);" id="endDate" class="date"><?php _p("today") ?></a>.
*/ ?>
            <script type="text/javascript">
<? /*
            Calendar.setup(
              {
                dateField: 'startDate',
                triggerElement: 'startDate',
                selectHandler: changeStartDate
              }
            );
*/ ?>
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
