google.load('visualization', '1.0', {'packages' : ['corechart', 'controls', 'treemap']});
google.setOnLoadCallback(drawCharts);

// Data
var componentData;
// Charts
var treemap, perComponent, timeDashboard, timeControl, perEdt;
// Chart options
var treemapOptions = {
	minColor : '#33d',
	midColor : '#ddd',
	maxColor : 'red',
	headerHeight : 15,
	fontColor : 'black',
	showScale : true,
	maxDepth : 1,
	maxPostDepth : 2,
	hintOpacity : 0.2,
	title: 'Energy map of board'
};

/**
 * Set up the charts and do the initial draw. Called on load
 * @return none
 */
function drawCharts() {
	// Create the treemap
	componentData = treemapQuery();
	treemap = new google.visualization.TreeMap(document
			.getElementById('treemap'));
	google.visualization.events.addListener(treemap, 'select', onTreemapSelect);
	google.visualization.events.addListener(treemap, 'rollup', onTreemapRollup);
	google.visualization.events.addListener(treemap, 'onmouseover', onTreemapMouseover);
	google.visualization.events.addListener(treemap, 'onmouseout', onTreemapMouseOut);
	treemap.draw(componentData, treemapOptions);

	// Create the energy/component column chart
	perComponent = new google.visualization.ChartWrapper( {
		chartType: 'ColumnChart',
		containerId: 'energyPerComponent',
		dataTable: componentData,
		options: {
			hAxis: {
				slantedTextAngle: 90
			},
			vAxis: {
				format: '##0.###E0'
			},
			legend: {'position': 'none'},
			// Options that allow coloring to work
			isStacked: true,
			series: {
				1: {
					color: 'FF0A0D',
					visibleInLegend: false
				}
			},
			title: 'Energy usage per XE (pJ)'
		}
	});
	// All XE rows
	perComponent.setView( {
		rows: componentData.getFilteredRows( [{
			column: 0,
			minValue: 'XE',
			maxValue: 'XF'
		}]),
		columns: [0, 3]
	});
	perComponent.draw();

	var pcl = google.visualization.events.addListener(perComponent, 'ready',
			function() {
				google.visualization.events.addListener(
						perComponent.getChart(), 'select', onXeSelect);
				google.visualization.events.removeListener(pcl);
			});
	perComponent.draw();

	// Create a dashboard for the energy/time chart and its filter control
	timeDashboard = new google.visualization.Dashboard(document
			.getElementById('energyOverTime_box'));

	// Control for the dashboard
	timeControl = new google.visualization.ControlWrapper( {
		controlType: 'ChartRangeFilter',
		containerId: 'energyOverTimeControl',
		options: {
			filterColumnIndex: 0,
			ui: {
				chartType: 'LineChart',
				chartOptions: {
					chartArea: { width: '90%' },
					vAxis: { baseline: 0 }
				}
			}
		}
	});

	// Data and chart for the dashboard
	timeData = timeQuery();
	overTime = new google.visualization.ChartWrapper({
		chartType: 'LineChart',
		containerId: 'energyOverTime',
		options: {
			// Width needs to be the same as it is for timeControl
			chartArea: {height: '80%', width: '90%'},
			legend: {position: 'none'},
			title: 'Power consumption over time (\u00B5W)',
			vAxis: { baseline: 0, format: '##0.###E0' }
		}
	});
	google.visualization.events.addListener(timeControl, 'statechange', onTimeChange);

	timeDashboard.bind(timeControl, overTime);
	timeDashboard.draw(timeData);

	// Create the energy/EDT column chart
	edtData = edtQuery();
	perEdt = new google.visualization.ChartWrapper({
		chartType: 'ColumnChart',
		containerId: 'energyPerEdt',
		dataTable: edtData,
		options: {
			legend: {position: 'none'},
			title: 'Energy usage per EDT (pJ)',
			vAxis: { baseline: 0, format: '##0.###E0' }
		}
	});

	var pel = google.visualization.events.addListener(perEdt, 'ready', function() {
		google.visualization.events.addListener(perEdt.getChart(), 'select',
				onEdtSelect);
		google.visualization.events.removeListener(pel);
	});
	perEdt.draw();
}

/**
 * @param maintainView True to maintain the treemap view and time range, false to reset
 */
function redrawCharts(maintainView) {
	// Ensure that maintainView is a bool
	maintainView = maintainView == true ? true : false;

	var treemapView, timeRange;
	if (maintainView) {
		treemapView = treemap.getSelection().length > 0 ? treemap
				.getSelection() : treemapRoot();
		timeRange = getTimeRange();
	} else {
		treemapView = treemapRoot();
		timeRange = null;
	}

	var componentName = componentData.getValue(treemapView[0].row,0);
	var component = formatComponent(componentName);
	if (maintainView) {
		data = overTime.getDataTable();
	}
	else {
		data = timeQuery(component);
	}
	timeDashboard.draw(data);
	if (!maintainView) {
		// Set the treemap view to be the root and the time range to be
		// everything for the remaining queries
		resetCharts();
	}

	var timeRangeCopy;
	if (!maintainView) {
		timeRangeCopy = timeRange === null ? null : {
			start: timeRange.start,
			end: timeRange.end
		};
		componentData = treemapQuery(timeRangeCopy);
	}
	treemap.draw(componentData, treemapOptions);
	if (maintainView) {
		treemap.setSelection(treemapView);
	}

	perComponent.setDataTable(componentData);
	XErows = getXEsUnder(componentName);
	perComponent.setView({
		rows: XErows,
		columns: [0, 3]
	});
	perComponent.draw();

	if (!maintainView) {
		timeRangeCopy = timeRange === null ? null : {
			start: timeRange.start,
			end: timeRange.end
		};
		perEdt.setDataTable(edtQuery(timeRangeCopy, component));
	}
	perEdt.draw();
}

function resetCharts() {
	treemap.setSelection(treemapRoot());
	timeRange = timeControl.getDataTable().getColumnRange(0);
	timeControl.getState().range.start = timeRange.min;
	timeControl.getState().range.end = timeRange.max;
}

/**
 * Gets the row in the data table of the first XE under the specified component
 * @param component The component to search children for (row ID or name)
 * @return
 */
function getXEOffset(component) {
	if (component === undefined)
	{
		if (treemap.getSelection().length == 0) {
			component = componentData.getFilteredRows([{column: 1, value: null}])[0];
		}
		else component = treemap.getSelection()[0].row;
	}
	if (typeof component == 'number') {
		component = componentData.getValue(component, 0);
	}
	return getXEsUnder(component)[0];
}

/**
 * For a given component, find all XEs that are a subcomponent
 * @param componentName The component to find XEs under
 * @type Array
 * @return A list of row indexes where XEs appear in componentData
 */
function getXEsUnder(componentName) {
	if (typeof componentName !== 'string') {
		console.error('Error: argument must be a string');
		return [];
	}

	var children = componentData.getFilteredRows( [{
		column: 1,
		value: componentName
	}]);
	if (componentName.match('XE')) {
		return componentData.getFilteredRows( [{
			column: 0,
			value: componentName
		}]);
	} else if (componentName.match('Block')) {
		return children;
	} else {
		var rv = [];
		for ( var i in children) {
			var grandchildren = getXEsUnder(componentData.getValue(children[i], 0));
			rv = rv.concat(grandchildren);
		}
		return rv;
	}
}

/**
 * Changes the color of a select list of rows in the energy/component chart
 * @param rows
 *            The list of row indices for the bars to change. The row indices
 *            should apply to the DataTable used for the treemap
 * @return none
 */
function colorSelection(rows) {
	rows = rows === undefined ? [] : rows;
	displayedRows = perComponent.getView().rows;
	if (rows.length > 0 && rows.length != displayedRows.length) {
		perComponent.setView( {
			rows: displayedRows,
			columns: [0, {
				type: 'number',
				label: componentData.getColumnLabel(3),
				calc: function(dt, row) {
					return (rows.indexOf(row) >= 0) ? null : {
						v: dt.getValue(row, 3),
						f: dt.getFormattedValue(row, 3)
					};
				}
			}, {
				type: 'number',
				label: componentData.getColumnLabel(3),
				calc: function(dt, row) {
					return (rows.indexOf(row) >= 0) ? {
						v: dt.getValue(row, 3),
						f: dt.getFormattedValue(row, 3)
					} : null;
				}
			}]
		});
	}
	else {
		perComponent.setView( {
			columns: [0, 3],
			rows: displayedRows
		});
	}
	perComponent.draw();
}

/**
 * Take the name of a component and put it into a format used by the database
 * queries
 *
 * @param componentName
 *            The name of the component
 * @return an object containing the chip, unit, etc. information
 */
function formatComponent(componentName) {
	var parts = componentName.split(' ');
	if (parts.length == 1) {
		return {};
	}
	parts = parts.pop().split('.');
	var component = {};
	if (parts.length > 0) {
		component.chip = parts.shift();
	}
	if (parts.length > 0) {
		component.unit = parts.shift();
	}
	if (parts.length > 0) {
		component.block = parts.shift();
	}
	if (parts.length > 0) {
		component.xe = parts.shift();
	}
	return component;
}

/**
 * Gets the current time range on the energy-over-time slider
 *
 * @return The current selected time range
 */
function getTimeRange() {
	var range = timeControl.getState().range;
	return {
		start: range.start,
		end: range.end
	};
}

/**
 * Returns the root of the treemap in the same format as getSelection()
 *
 * @return
 */
function treemapRoot() {
	return [{
		row: componentData.getFilteredRows( [{
			column: 1,
			value: null
		}])[0]
	}];
}

/* Event listeners */
function onTreemapSelect(e) {
	var row = treemap.getSelection()['0'].row;
	var componentName = componentData.getValue(row,0);
	var component = formatComponent(componentName);

	XErows = getXEsUnder(componentName);
	perComponent.setView({rows:XErows, columns:[0,3]});
	perComponent.draw();

	timeDashboard.draw(timeQuery(component));
	perEdt.setDataTable(edtQuery(getTimeRange(), component));
	perEdt.draw();
}

function onTreemapRollup(e) {
	var newView = componentData.getValue(e.row, 1);
	var component = formatComponent(newView);
	// Parent of row navigated FROM
	XErows = getXEsUnder(newView);
	perComponent.setView({rows:XErows, columns:[0,3]});
	perComponent.draw();

	timeDashboard.draw(timeQuery(component));
	perEdt.setDataTable(edtQuery(getTimeRange(), component));
	perEdt.draw();
}

function onTreemapMouseover(e) {
	rows = getXEsUnder(componentData.getValue(e.row, 0));
	offset = getXEOffset();
	var selection = [];
	for (i in rows) {
		selection = selection.concat({
			row: rows[i]-offset,
			column: 1
		});
	}
	colorSelection(rows);
}

function onTreemapMouseOut(e) {
	colorSelection();
}

function onXeSelect(e) {
	var selection = perComponent.getChart().getSelection()[0];
	if (selection === undefined) {
		return;
	}
	var view = perComponent.getView();
	var xe = perComponent.getDataTable().getValue(view.rows[selection.row],view.columns[0]);
	var energy = perComponent.getDataTable().getValue(view.rows[selection.row],view.columns[1]);
	var energy1 = energy*Math.random();
	var energy2 = energy-energy1;

	localStorage.setItem('xe_component', JSON.stringify(formatComponent(xe)));
	localStorage.setItem('xe_timeRange', JSON.stringify(getTimeRange()));
	var infoPopup = window.open('xeinfo.htm', 'XEDataPopup',
			'location=0,menubar=0,toolbar=0,height=400,width=650', true);
	infoPopup.focus();
}

function onEdtSelect(e) {
	var selection = perEdt.getChart().getSelection()[0];
	if (selection === undefined) {
		return;
	}
	var edt=perEdt.getDataTable().getValue(selection.row, 0);
	var row = (treemap.getSelection().length > 0 ? treemap.getSelection()
			: treemapRoot())[0].row;
	var componentName=componentData.getValue(row, 0);
	var component=formatComponent(componentName);
	var energy=perEdt.getDataTable().getValue(selection.row, 1);

	localStorage.setItem('edt_edt', edt);
	localStorage.setItem('edt_component', JSON.stringify(component));
	localStorage.setItem('edt_timeRange', JSON.stringify(getTimeRange()));
	var infoPopup = window.open('edtinfo.htm', 'edtDataPopup',
			'location=0,menubar=0,toolbar=0,height=400,width=650', true);
	infoPopup.focus();
}

function onTimeChange(e) {
	if (!e.inProgress) {
		var selection = treemap.getSelection().length > 0 ? treemap.getSelection() : treemapRoot();
		var component = formatComponent(componentData.getValue(selection['0'].row, 0));
		if (component == {}) component = null;
		var timeRange = getTimeRange();
		// Update treemap & component chart
		componentData = treemapQuery(timeRange);
		treemap.draw(componentData, treemapOptions);
		treemap.setSelection(selection);
		// Update component chart
		perComponent.setDataTable(componentData);
		perComponent.draw();
		// Update edt chart
		perEdt.setDataTable(edtQuery(getTimeRange(), component));
		perEdt.draw();
		// Update time chart
		newTimeData = timeQuery(component, getTimeRange());
		timeDashboard.draw(newTimeData);
	}
}
