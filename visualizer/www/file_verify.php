<?php
session_start();

print_r($_GET);
if (isset($_GET['path'])) {
	$path = $_GET['path'];
	if (file_exists($path)) {
		$_SESSION['path'] = $path;
	}
	else {
		$path = '';
	}
}
echo $path;

?>