<?php
session_start();

function varDumpToString ($var)
{
	ob_start();
	var_dump($var);
	$result = ob_get_clean();
	return $result;
}
function mylog($str)
{
	error_log($str, 3, 'readdb.log');
}

if (isset($_SESSION['path'])) {
	$dbname = $_SESSION['path'];
}
else {
	$dbname = ':memory:';
}
$tablename = 'edts';

function parseQuery($query) {
	// Get the fields to SELECT
	$fields = $query["fields"];
	$fieldsStr = implode(',', $fields);
	if (strlen($fieldsStr)==0) {
		$fieldsStr = "*";
	}

	// Get the restrictions on the query
	$restrictions = isset($query["restrictions"]) ? $query["restrictions"] : array();
	$restrictionsStr = array();
	// Component; chip/unit/block/XE values are integers.
	if (array_key_exists('component', $restrictions)) {
		foreach ($restrictions['component'] as $component => $value) {
			$restrictionsStr[] = $component.'='.$value;
		}
	}
	// EDT: Specified in the form edt="foo"
	if (array_key_exists('edt', $restrictions)) {
		$restrictionsStr[] = 'edt="' . $restrictions['edt'] . '"';
	}
	// Time range: If specified, it will always be both the start and end times.
	if (array_key_exists('timeRange', $restrictions)) {
		$restrictionsStr[] = 'start_time>=' . $restrictions['timeRange']['start'];
		$restrictionsStr[] = 'end_time<=' . $restrictions['timeRange']['end'];
	}
	$restrictionsStr = implode(' AND ', $restrictionsStr);

	// Check for filters on data, like order/group by or limit
	$filters = '';
	if (isset($query['filters'])) {
		foreach ($query['filters'] as $key => $value) {
			$values = gettype($value)=='array' ? implode(',',$value) : $value;
			$filters = $filters.' '.$key.' '.$values;
		}
	}

	if (array_key_exists('source', $query)) {
		// If the source is supplied, it should be a nested array of queries, resulting in a query such as:
		// SELECT foo FROM (<query1> UNION <query2>)
		// Where the nested query will be the portion inside paretheses.
		$nestedQueries = $query['source'];
		$nestedQueriesStr = array();
		foreach ($nestedQueries as $nq) {
			$nestedQueriesStr[] = parseQuery($nq);
		}
		$table = " (".implode(' UNION ', $nestedQueriesStr).") ";
	}
	else {
		$table = $GLOBALS['tablename'];
	}

	// Build a string for the query
	$queryStr = 'SELECT ' . $fieldsStr . ' FROM ' . $table;
	if (strlen($restrictionsStr) > 0) {
		$queryStr = $queryStr . ' WHERE ' . $restrictionsStr;
	}
	$queryStr = $queryStr . $filters;
	return $queryStr;
}

// Queries are sent in an array, usually of length 1. If there are multiple
// queries, they must be able to be joined with the UNION keyword.
$queries = $_GET["queries"];
$queriesStr = array();
foreach ($queries as $query) {
	$queriesStr[] = parseQuery($query);
}
$queriesStr = implode(' UNION ', $queriesStr);

// Get the labels and types for the column headers
$col_headers =$_GET["headers"];
$fields = array_keys($col_headers);

function buildRow($resultRow) {
	global $fields;
	$temp = array();
	foreach ($fields as $field) {

		$temp[] = array('v' => $resultRow[$field]);
	}
	return array('c' => $temp);
}

function makeTableRows($results) {
	if (isset($_GET['sample'])) {
		$sample = $_GET['sample'];
		// Verify contents exist: what field to limit or sum,
		// and max number of entries. Start and end of range
		// can be left out.
		if (!isset($sample['sampled']) ||
			!isset($sample['summed']) ||
			!isset($sample['maxEntries'])) {
			unset($sample);
		}
	}

	$rows = array();
	if (isset($sample)) {
		$sumField = $sample['summed'];
		$start = array_shift(array_values($results));
		$start = $start[$sample['sampled']];
		$end = array_pop(array_values($results));
		$end = $end[$sample['sampled']];
		$duration = $end-$start;
		$step = $duration / $sample['maxEntries'];

		if (!isset($sample['start']) || !isset($sample['end']))
		{
			$sample['start'] = $start;
			$sample['end'] = $end;
		}
		$subStart = $sample['start'];
		$subEnd = $sample['end'];
		$subStep = ($subEnd - $subStart) / $sample['maxEntries'];

		$min_time = 0;
		$to_skip = array();
		foreach ($results as $r) {
			$time = $r['time'];
			if ($time < $min_time) {
				$to_skip[] = $r;
			}
			else {
				if (count($to_skip) > 0) {
					// If the current one is far enough away from the last one,
					// put the sum in the last one to unset and keep it.
					$to_add = array_pop($to_skip);
					foreach ($to_skip as $skip) {
						$to_add[$sumField] += $skip[$sumField];
					}

					$rows[] = buildRow($to_add);
					$to_skip = array();
				}
				$rows[] = buildRow($r);
			}
			if ($time >= $subStart && $time <= $subEnd) {
				$min_time = $time + $subStep;
			}
			else {
				$min_time = $time + $step;
			}
		}
	}
	else {
		foreach ($results as $r) {
			$rows[] = buildRow($r);
		}
	}
	return $rows;
}

try {
	// Open the database
	$conn = new PDO("sqlite:$dbname");
	$conn->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

	if ($dbname == ':memory:') {
		// Special case: Need to create the table so it can return something empty instead of throwing an error.
		$conn->exec('CREATE TABLE edts(
			id INTEGER PRIMARY KEY,
			chip INTEGER,
			unit INTEGER,
			block INTEGER,
			xe INTEGER,
			edt TEXT,
			start_time INTEGER,
			end_time INTEGER,
			energy REAL)');
	}

	$result = $conn->query($queriesStr);

	// Build a table to show the SQL result in Google Chart format
	// Format: https://developers.google.com/chart/interactive/docs/reference#dataparam
	$rows = array();
	$table = array();

	// Build column list based on $fields
	$table['cols'] = array();
	foreach ($fields as $field) {
		$table['cols'][] = $col_headers[$field];
	}

	// Create a row for each entry
	$rows = makeTableRows($result->fetchAll());

	// Convert the table to the JSON format that Google Charts needs
	$table['rows'] = $rows;
	$jsonTable = json_encode($table);
	echo $jsonTable;

} catch (PDOException $e) {
	echo 'ERROR: ' . $e->getMessage() . '</br>';
	echo $e->getTraceAsString();
}
?>