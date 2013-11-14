// dbquery.js
// Contains functions to send SQL queries with JQuery/AJAX

// Create a couple conversion constants
var timeScale = '1.0e6'; // us to s
var energyScale = '1.0e-12'; // pJ to J
/**
 * Generates and submits a SQL query to get per-component energy data for a
 * given time range using $.ajax
 *
 * @param timeRange
 *            array of two non-negative integers to indicate the relevant
 *            timespan
 * @return The JSON text result
 */
function componentQuery(timeRange, component) {
	var restrictions = {};
	// Check type of timeRange
	if (timeRange != null) {
		if (isNaN(parseInt(timeRange.start)) || isNaN(parseInt(timeRange.end))) {
			console
					.error("Invalid value for time range. Defaulting to full time range.");
		} else {
			timeRange.start *= parseFloat(timeScale);
			timeRange.end *= parseFloat(timeScale);
			restrictions.timeRange = timeRange;
		}
	}
	if (component != null) {
		restrictions.component = component;
	}

	// Query data contains 3 items: fields (SELECT), restrictions (WHERE), and
	// filters
	var queryData = {
		fields: ['chip||"."||unit||"."||block||"."||xe AS component',
				'SUM(energy) AS energyTotal'],
		restrictions: restrictions,
		filters: {
			"GROUP BY": ['component']
		}
	};
	var queries = {
		queries: [queryData],
		headers: {
			'component': {
				label: 'Component',
				type: 'string'
			},
			'energyTotal': {
				label: 'Energy',
				type: 'number'
			}
		}
	};

	return sendQuery(queries);
}

/**
 * Wrapper around componentQuery for the treemap, selecting all components
 *
 * @param queryType
 * @param timeRange
 * @return The JSON test result
 */
function treemapQuery(timeRange) {
	var data = componentQuery(timeRange);

	// If the time range is small enough, some XEs may have no data. Add them.
	var xedata = sendQuery(
			{
				queries: [{
					fields: ['DISTINCT chip||"."||unit||"."||block||"."||xe AS component']
				}],
				headers: {
					'component': {
						label: 'xe',
						type: 'string'
					}
				}
			}).getDistinctValues(0); // All XEs
	xelist = data.getDistinctValues(0); // Included XEs
	for ( var xe in xedata) {
		if (xelist.indexOf(xedata[xe]) < 0) {
			data.addRow([xedata[xe], 0]);
		}
	}

	// Insert "parent" column
	data.insertColumn(1, 'string', 'Parent');
	// Insert "size" column
	data.insertColumn(2, 'number', 'size');

	// Set parents and sizes appropriately
	var xecount = data.getNumberOfRows();
	for ( var i = 0; i < xecount; i++) {
		// This REALLY should not be necessary.
		data.setCell(i, 3, parseFloat(data.getValue(i, 3)));

		// Each XE has a size of 1
		data.setCell(i, 2, 1);
		// Add "XE " prefix to name, then add rows for parents
		var componentName = data.getValue(i, 0);
		data.setCell(i, 0, 'XE ' + componentName);

		var parent = componentName.split('.');
		var xe = parent.pop();
		data.setCell(i, 1, 'Block ' + parent.join('.'));

		if (xe == 0) {
			var name = 'Block ' + parent.join('.');
			var block = parent.pop();
			data.addRow([name, 'Unit ' + parent.join('.'), 0, 0]);
			if (block == 0) {
				name = 'Unit ' + parent.join('.');
				var unit = parent.pop();
				data.addRow([name, 'Chip ' + parent.join('.'), 0, 0]);
				if (unit == 0) {
					data.addRow(['Chip ' + parent.join('.'), 'Board', 0, 0]);
				}
			}
		}
	}
	data.addRow(['Board', null, 1, 0]);

	return data;
}

/**
 * Generates and submits a SQL query to get per-EDT energy data for a given time
 * range using $.ajax
 *
 * @param timeRange
 *            array of two non-negative integers to indicate the relevant
 *            timespan
 * @return The JSON text result
 */
function edtQuery(timeRange, component) {
	var restrictions = {};
	// Check type of timeRange
	if (timeRange != null) {
		if (isNaN(parseInt(timeRange.start)) || isNaN(parseInt(timeRange.end))) {
			console
					.error("Invalid value for time range. Defaulting to full time range.");
		} else {
			timeRange.start *= parseFloat(timeScale);
			timeRange.end *= parseFloat(timeScale);
			restrictions.timeRange = timeRange;
		}
	}
	if (component != null) {
		restrictions.component = component;
	}
	// Query data contains 3 items: fields (SELECT), restrictions (WHERE), and
	// GROUP BY
	var queryData = {
		fields: ['edt', 'SUM(energy) AS energyTotal'],
		restrictions: restrictions,
		filters: {
			"GROUP BY": ['edt']
		}
	};
	var queries = {
		queries: [queryData],
		headers: {
			'edt': {
				label: 'EDT',
				type: 'string'
			},
			'energyTotal': {
				label: 'Energy',
				type: 'number'
			}
		}
	};

	return sendQuery(queries);
}

function timeQuery(component, timeRange) {
	var restrictions = {};
	if (component != null) {
		restrictions.component = component;
	}
	// There are two queries: one for increases in energy at the start of an
	// EDT, one for decreases in energy at the end of an EDT
	var query1 = {
		fields: ['start_time AS time',
				'SUM(energy/(end_time-start_time)) AS powerDelta'],
		restrictions: restrictions,
		filters: {
			"GROUP BY": ['time']
		}
	};
	var query2 = {
		fields: ['end_time AS time',
				'-SUM(energy/(end_time-start_time)) AS powerDelta'],
		restrictions: restrictions,
		filters: {
			"GROUP BY": ['time']
		}
	};
	var innerQueries = [query1, query2];

	// The above is a subquery. It needs to be inside
	// SELECT time,powerDelta FROM (innerQueries) GROUP BY time
	var outerQuery = {
		fields: [
				'time/' + timeScale + ' as time',
				'SUM(powerDelta)*' + energyScale + '*' + timeScale
						+ ' AS power'],
		filters: {
			"GROUP BY": ['time']
		},
		source: innerQueries
	};

	var queries = {
		queries: [outerQuery],
		headers: {
			'time': {
				label: 'Time',
				type: 'number'
			},
			'power': {
				label: 'Power',
				type: 'number'
			}
		}
	};

	// Create sampling data
	var sample = {
		sampled: 'time',
		summed: 'power',
		maxEntries: document.getElementById('energyOverTime').offsetWidth
	};
	if (timeRange != null) {
		sample.start = timeRange.start;
		sample.end = timeRange.end;
	}
	queries.sample = sample;

	var dataTable = sendQuery(queries, timeRange);
	console.log("Read " + dataTable.getNumberOfRows() + " rows");
	for ( var i = 0; i < dataTable.getNumberOfRows(); i++) {
		// This REALLY should not be necessary.
		dataTable.setCell(i, 0, parseFloat(dataTable.getValue(i, 0)));
		dataTable.setCell(i, 1, parseFloat(dataTable.getValue(i, 1)));
	}

	var c = 1; // Energy is in column 1
	// Get a cumulative sum of the energy column
	for ( var r = 1; r < dataTable.getNumberOfRows(); r++) {
		dataTable.setCell(r, c, parseFloat(dataTable.getValue(r - 1, c))
				+ parseFloat(dataTable.getValue(r, c)));
	}

	// Add a second row for each time value except the first and last to achieve
	// the square wave effect
	for ( var r = 1; r < dataTable.getNumberOfRows(); r += 2) {
		var time = parseFloat(dataTable.getValue(r, 0));
		var energy = dataTable.getValue(r - 1, c);
		dataTable.insertRows(r, [[time, energy]]);
	}

	// And because rounding errors drive me nuts...
	// var formatter = new google.visualization.NumberFormat( {
	// fractionsDigits: 3
	// });
	// formatter.format(dataTable, 1);

	return dataTable;
}

// Queries for popup tables
function clickXeQuery(component, timeRange) {
	var restrictions = {};
	if (timeRange != null) {
		if (isNaN(parseInt(timeRange.start)) || isNaN(parseInt(timeRange.end))) {
			console
					.error("Invalid value for time range. Defaulting to full time range.");
		} else {
			timeRange.start *= parseFloat(timeScale);
			timeRange.end *= parseFloat(timeScale);
			restrictions.timeRange = timeRange;
		}
	}
	if (component != null) {
		restrictions.component = component;
	}
	var queryData = {
		fields: ['start_time/' + timeScale + ' AS start_time',
				'end_time/' + timeScale + ' AS end_time',
				'(end_time-start_time)/' + timeScale + ' AS duration', 'edt',
				'energy'],
		restrictions: restrictions,
		filters: {
			"ORDER BY": ['start_time'],
			"LIMIT": 10000
		}
	};
	var query = {
		queries: [queryData],
		headers: {
			'start_time': {
				label: 'Start time (s)',
				type: 'number'
			},
			'end_time': {
				label: 'End time (s)',
				type: 'number'
			},
			'duration': {
				label: 'Duration (s)',
				type: 'number'
			},
			'edt': {
				label: 'EDT',
				type: 'string'
			},
			'energy': {
				label: 'Energy (pJ)',
				type: 'number'
			}
		}
	};

	return sendQuery(query);
}

function clickEdtQuery(edt, component, timeRange) {
	var restrictions = {
		edt: edt
	};
	if (timeRange != null) {
		if (isNaN(parseInt(timeRange.start)) || isNaN(parseInt(timeRange.end))) {
			console
					.error("Invalid value for time range. Defaulting to full time range.");
		} else {
			timeRange.start *= parseFloat(timeScale);
			timeRange.end *= parseFloat(timeScale);
			restrictions.timeRange = timeRange;
		}
	}
	if (component != null) {
		restrictions.component = component;
	}
	var queryData = {
		fields: ['start_time/' + timeScale + ' AS start_time',
				'end_time/' + timeScale + ' AS end_time',
				'(end_time-start_time)/' + timeScale + ' AS duration',
				'chip||"."||unit||"."||block||"."||xe AS component', 'energy'],
		restrictions: restrictions,
		filters: {
			"ORDER BY": ['start_time'],
			"LIMIT": 10000
		}
	};
	var query = {
		queries: [queryData],
		headers: {
			'start_time': {
				label: 'Start time (s)',
				type: 'number'
			},
			'end_time': {
				label: 'End time (s)',
				type: 'number'
			},
			'duration': {
				label: 'Duration (s)',
				type: 'number'
			},
			'component': {
				label: 'XE',
				type: 'string'
			},
			'energy': {
				label: 'Energy (pJ)',
				type: 'number'
			}
		}
	};

	return sendQuery(query);
}

/**
 * Submit a SQL query using a data object that optionally contains an array of
 * fields, an object of restrictions, and an array of fields to group/order by
 *
 * @param data
 * @return
 */
function sendQuery(data) {
	var jsonData = $.ajax({
		url: "readdb.php",
		data: data,
		dataType: "json",
		async: false
	}).responseText;
	return new google.visualization.DataTable(jsonData);
}
