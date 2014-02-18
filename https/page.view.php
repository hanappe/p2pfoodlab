<!DOCTYPE html>
<html>
  <head>
    <title>P2P Food Lab</title>
    <script type="text/javascript" src="<?php url('config.js') ?>"></script>
    <script type="text/javascript" src="<?php url('p2pfoodlab.js') ?>"></script>
    <link rel="stylesheet" type="text/css" href="<?php url('p2pfoodlab.css') ?>" />
    <script type="text/javascript" src="<?php url('slideshow.js') ?>"></script>
    <link rel="stylesheet" type="text/css" href="<?php url('slideshow.css') ?>" />
    <script type="text/javascript" src="<?php url('dygraph-combined.js') ?>"></script>
    <script type="text/javascript" src="<?php url('json2.js') ?>"></script>
    <script type="text/javascript" src="<?php url('md5.js') ?>"></script>
    <script type="text/javascript" src="<?php url('opensensordata.js') ?>"></script>
    <link rel="stylesheet" type="text/css" href="<?php url('calendarview.css') ?>" />
    <script type="text/javascript" src="<?php url('prototype.js') ?>"></script>
    <script type="text/javascript" src="<?php url('calendarview.js') ?>"></script>
  </head>

  <body>
    <div class="screen">

<?php if ($this->show_menu): ?>
      <div class="strip">
        <div class="content_box">
          <div class="header">

            <div class="header_item frame margin">
              <div class="menu">
                <?php $this->insert_title() ?>
              </div>
            </div>

<?php if (isset($this->host)): ?>
            <div class="header_item">
                <?php $this->insert_notebook_menu() ?>
            </div>

            <div class="header_item">
                <?php $this->insert_greenhouse_menu() ?>
            </div>
<?php endif; ?>

            <div class="header_item header_last">
              <div class="menu">
                <a href="<?php url('index') ?>" class="menu"><img src="<?php url('favicon-blue.png') ?>" class="logo" />P2P Food Lab</a>
              </div>
<?php if ($this->show_account_submenu) {
        if ($this->visitor == NULL): ?>
              <div class="submenu shift_logo">
                <a href="<?php url('login') ?>" class="menu"><?php _p("login") ?></a> 
                - 
                <a href="<?php url('createaccount') ?>" class="menu"><?php _p("join") ?></a> 
                -
                <a href="<?php url('help') ?>" class="menu"><?php _p("help") ?></a> 
              </div>
<?php else: ?>
              <div class="submenu shift_logo">
                <span class="blue"><?php echo $this->visitor->username ?></span>&nbsp;
                <a href="<?php url('people/' . $this->visitor->username) ?>" class="menu"><?php _p("home") ?></a> 
                -
                <a href="<?php url('people/' . $this->visitor->username . '/account') ?>" class="menu"><?php _p("account") ?></a> 
                -
                <a href="<?php url('logout') ?>" class="menu"><?php _p("logout") ?></a> 
                -
                <a href="<?php url('help') ?>" class="menu"><?php _p("help") ?></a> 
              </div>
<?php endif; } ?>
            </div>

          </div>
        </div>

      </div> <!-- strip  -->
<?php endif; ?>

      <?php $this->insert_body()  ?>

      <div class="strip">
        <div class="content_box license">
          Licensed under <a href="http://creativecommons.org/licenses/by-sa/4.0/legalcode">Creative Commons Attribution-ShareAlike (CC BY-SA)</a>.
        </div>
      </div> <!-- strip  -->

    </div> <!-- screen  -->
  </body>
</html>
