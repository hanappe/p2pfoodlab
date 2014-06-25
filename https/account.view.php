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

$interface_lang = get_interface_languages();
$most_used = get_most_used_languages();
$lang = get_languages();
?>
  
<div class="strip">
  <div class="content_box frame">
    <div class="margin">
      <form action="" method="post" name="set_locale">
        <p><?php _p("Language settings - Interface") ?> (<a href="<?php url("help#settings:language")?>"><?php _p("help")?></a>)</p>
        <?php _p("Please select the language for the web site interface:") ?>
        <select name="lang" onchange="updateInterfaceLanguage()">
          <?php
             echo $this->visitor->locale;

          for ($i = 0; $i < count($interface_lang); $i++) {
                            $code = $interface_lang[$i]->code;
            $english = $interface_lang[$i]->english;
            $native = $interface_lang[$i]->native;
            $selected = ($this->visitor->locale == $code)? 
            " selected=\"selected\"" : "";
            echo "            <option value=\"$code\" $selected>$english -- $native</option>\n";
            }
            ?>
        </select>
      </form>
    </div> 
  </div> 
</div> 


<div class="strip">
  <div class="content_box frame">

    <div class="margin visible" id="lang_short">

      <p><?php _p("Language settings - Notebook") ?> (<a href="<?php url("help#settings:language")?>"><?php _p("help")?></a>)</p>

      <p>
        <form>
          <?php _p("Default language:") ?> 
          <select name="preflang">
            <?php
               for ($i = 0; $i < count($this->visitor->lang); $i++) {
            echo "               <option value='" . $this->visitor->lang[$i] . "'>" . $this->visitor->lang[$i] . "</option>\n";
            }
            ?>
          </select>
        </form>
      </p>

      <p><a href="javascript:void(0);" onclick="show_hide('lang')"><?php _p("Click to view all languages") ?></a></p>
      
      <p>
        <form action="" method="post" name="set_lang_short">
          <table class="languages">
            <?php
               for ($i = 0; $i < count($most_used); $i += 2) {
                                 echo "        <tr>\n";

                                 $code = $most_used[$i]->code;
               $english = $most_used[$i]->english;
            $native = $most_used[$i]->native;
            $selected = $this->visitor->supports_lang($code)? " checked=\"checked\"" : "";
            echo "            <td class='language_check'><input type='checkbox' name='short_$code' value=\"$code\" id=\"lang_$code\" $selected onchange=\"updateLanguage('$code','short_$code')\" /><label for='lang_$code'>[$code]</label></td>\n";
            echo "            <td class='language'><label for='lang_$code'><span class='blue'>$native</span></label><br><span class='small'>$english</span></td>\n";

            if ($i+1 < count($most_used)) {
                       $code = $most_used[$i+1]->code;
              $english = $most_used[$i+1]->english;
              $native = $most_used[$i+1]->native;
              $selected = $this->visitor->supports_lang($code)? " checked=\"checked\"" : "";
              echo "            <td class='language_check'><input type='checkbox' name='short_$code' value=\"$code\" id=\"lang_$code\" $selected onchange=\"updateLanguage('$code','short_$code')\" /><label for='lang_$code'>[$code]</label></td>\n";
              echo "            <td class='language'><label for='lang_$code'><span class='blue'>$native</span><br><span class='small'>$english</span></label></td>\n";
              } else {
              echo "            <td></td><td></td>\n";
              }

              echo "        </tr>\n";
}
?>
          </table>
        </form>
      </p> 
    </div> 

    
    <div class="margin hidden" id="lang">

      <p><?php _p("Language settings - Notebook") ?> (<a href="<?php url("help#settings:language")?>"><?php _p("help")?></a>)</p>
      
      <p>
        <form>
          <?php _p("Default language:") ?> 
          <select name="preflang">
            <?php
               for ($i = 0; $i < count($this->visitor->lang); $i++) {
            echo "               <option value='" . $this->visitor->lang[$i] . "'>" . $this->visitor->lang[$i] . "</option>\n";
            }
            ?>
          </select>
        </form>
      </p>

      <p><a href="javascript:void(0);" onclick="show_hide('lang')"><?php _p("Click to reduce the number of languages") ?></a></p>
      
      <p>
        <form action="" method="post" name="set_lang_short">
          <table class="languages">

            <?php
               for ($i = 0; $i < count($lang); $i += 2) {
                                 echo "        <tr>\n";

                                 $code = $lang[$i]->code;
               $english = $lang[$i]->english;
            $native = $lang[$i]->native;
            $selected = $this->visitor->supports_lang($code)? " checked=\"checked\"" : "";
            echo "            <td class='language_check'><input type='checkbox' name='short_$code' value=\"$code\" id=\"lang_$code\" $selected onchange=\"updateLanguage('$code','short_$code')\" /><label for='lang_$code'>[$code]</label></td>\n";
            echo "            <td class='language'><label for='lang_$code'><span class='blue'>$native</span></label><br><span class='small'>$english</span></td>\n";

            if ($i+1 < count($lang)) {
                       $code = $lang[$i+1]->code;
              $english = $lang[$i+1]->english;
              $native = $lang[$i+1]->native;
              $selected = $this->visitor->supports_lang($code)? " checked=\"checked\"" : "";
              echo "            <td class='language_check'><input type='checkbox' name='short_$code' value=\"$code\" id=\"lang_$code\" $selected onchange=\"updateLanguage('$code','short_$code')\" /><label for='lang_$code'>[$code]</label></td>\n";
              echo "            <td class='language'><label for='lang_$code'><span class='blue'>$native</span><br><span class='small'>$english</span></label></td>\n";
              } else {
              echo "            <td></td><td></td>\n";
              }

              echo "        </tr>\n";
}
?>
          </table>
        </form>
      </p> 
    </div> 
  
  </div> 
</div> 

<div class="strip">
  <div class="content_box frame">
    <div class="margin">
      <p>Your account info at OpenSensorData.net:</p>

      <p>Username: <?php echo $this->visitor->osd_username ?></p>
      <p>User ID: <?php echo $this->visitor->osd_id ?></p>
      <p>Account key: <?php echo $this->visitor->osd_key ?></p>
      
    </div> 
  </div> 
</div> 

<?php if ($this->osd_account && (count($this->osd_account->groups) > 0)): ?>
<div class="strip">
  <div class="content_box frame">
    <div class="margin">
      <p>Sensorboxes:</p>

      <script>
        var _groups = <?php echo json_encode($this->osd_account->groups) ?>;
        var _datastream = <?php echo json_encode($this->datastreams) ?>;
        var _visible = <?php echo json_encode($this->visible) ?>;
      </script>
      <div>
        <form action="#" name="groupselector" >
          <select name="group" onchange="showgroup()">
            <?php 
               for ($i = 0; $i < count($this->osd_account->groups); $i++) {
            echo "          <option value=\"" . $this->osd_account->groups[$i]->id . "\">" . $this->osd_account->groups[$i]->name . "</option>\n";
            }
            ?>
          </select>
          show? <input type="checkbox" name="show" onchange="changegroup()" />
        </form>
      </div>
      <div class="margin">
        Visible datastreams:
        <div> 
          <form action="#" name="dataselector" >
          </form>
        </div> 
      </div> 

      <script>showgroup();</script>

    </div> 
  </div> 
</div> 
<?php endif; ?>

<?php
$html = $this->visitor->get_homepage();
?>
<div class="strip">
  <div class="content_box frame">
    <div class="margin">
      <p><?php _p("You can edit your homepage below. Just use plain HTML.") ?></p>
      <form action="" method="post" name="homepage">
         <textarea name="html"><?php echo htmlspecialchars($html) ?></textarea>
      </form>
      <p><a href="javascript:void(0)" onclick="updateHomepage()"><?php _p("Update my homepage.") ?></a></p>
    </div> 
  </div> 
</div> 

