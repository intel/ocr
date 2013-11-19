<!DOCTYPE html>
<html>
<?php session_start(); ?>
<head>
	<link rel="stylesheet" type="text/css" href="style.css">
	<!--Load the AJAX API-->
	<script type="text/javascript" src="https://www.google.com/jsapi"></script>
	<script type="text/javascript" src="//ajax.googleapis.com/ajax/libs/jquery/1.10.2/jquery.min.js"></script>
	<script type="text/javascript" src="dbquery.js"></script>
	<script type="text/javascript" src="charts.js"></script>
	<script type="text/javascript">
	$(document).ready(function() {
		$(document).keydown(function(e) {
			if (e.keyCode == 27) { // Esc
				hideLightbox();
				document.getElementById('dbSelectBtn').focus();
			}
		});
	});

	function showLightbox() {
        document.getElementById('light').style.display='block';
        document.getElementById('fade').style.display='block';
	}
	function hideLightbox() {
		document.getElementById('light').style.display='none';
        document.getElementById('fade').style.display='none';
	}
	function updatePath() {
		// Check to make sure database exists
		$.ajax({
			type: "GET",
			cache: false,
			url: "file_verify.php",
			datatype: "html",
			data: {
				path: document.getElementById('db_path').value
			},
			success: function(data) {
				redrawCharts();
			}
		});
		hideLightbox();
	}
	function checkKeyPress(e) {
		if (typeof e == 'undefined' && window.event) { e = window.event; }
		if (e.which == 13)
		{
			document.getElementById('db_submit').click();
		}
	}
	window.onresize = function(event) {
		redrawCharts(true);
	};
	</script>
	<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
	<title>OCR Energy Usage</title>
</head>
<body>
	<div id="dbSelection">
		<div>
		<input id="dbSelectBtn" type="button" value="Select database" onclick="showLightbox(); document.getElementById('db_path').focus()">
		</div>
	</div>
	<div>
		<div id="treemap_box">
		<div id="treemap" class="chart"></div>
		</div>
		<div id="energyOverTime_box" class="chart">
			<div id="energyOverTime"></div>
			<div id="energyOverTimeControl" style="height: 50px"></div>
		</div>
		<div id="subchartsDashboard">
			<div id="energyPerComponent" class="chart" style="float:left; width: 50%">
			</div>
			<div id="energyPerEdt" class="chart" style="float:right; width: 50%">
			</div>
		</div>
	</div>
	<div id="light" class="lightbox_content">
	Enter the location of your database file on the server.
	<br/>
	<input type="text" id="db_path" onkeypress="checkKeyPress(event);">
	<input type="button" id="db_submit" onclick="updatePath()" value="Select">
	<div class="footer">
	<a href="javascript:void(0)" onclick="hideLightbox()">Click to dismiss</a>
	</div>
	</div>
	<div id="fade" class="overlay"></div>
</body>
</html>
