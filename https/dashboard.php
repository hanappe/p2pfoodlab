<!DOCTYPE html>
<html>
<head>
    <script src="easeljs-0.7.1.min.js"></script>
    <script>
        function init() {
                 var stage = new createjs.Stage("demoCanvas");
                 var circle = new createjs.Shape();
                 circle.graphics.beginFill("red").drawCircle(0, 0, 50);
                 circle.x = 100;
                 circle.y = 100;
                 stage.addChild(circle);
        }
    </script>
</head>
<body onLoad="init();">
    <canvas id="demoCanvas" width="500" height="300">
        alternate content
    </canvas>
</body>
</html>

