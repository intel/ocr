#!/usr/bin/python
'''
Created on Oct 11, 2013

@author: Glen Haber
'''

import sys
import sqlite3
import argparse

EDT_TABLE_NAME = 'edts'
TIME_TABLE_NAME = 'time'

events = dict()
cores = dict()

def mapFromFile(filename):
    cores = set()
    for line in open(filename):
        if line.find('EDT start') > 0:
            cores.add(line.split()[3])
    return cores

def gen_core_map(logfile):
    '''Generates the core map based on the log file'''
    coreSet = mapFromFile(logfile)
    component = [0,0,0,0]
    XEMAX = 7
    BLOCKMAX = 15
    UNITMAX = 15
    # Assume infinite chips until we consider multiple boards
    def incComponent():
        if component[3] < XEMAX:
            component[3] += 1
        else:
            component[3] = 0
            if component[2] < BLOCKMAX:
                component[2] += 1
            else:
                component[2] = 0
                if component[1] < UNITMAX:
                    component[1] += 1
                else:
                    component[1] = 0
                    component[0] += 1
    for core in coreSet:
        cores[core] = component[:]
        incComponent()

def read_core_map(core_map_file):
    '''Generates the core map based on a supplied map'''
    with open(core_map_file) as f:
        lines = [line.strip() for line in f]
    for line in lines:
        [guid, name] = line.split('|')
        cores[guid] = name.split('.')

def parse_edt_line(terms):
    time = int(terms[0])
    event_type = terms[2].strip(':') # value will be 'start' or 'end'
    core_guid = terms[3]
    edt_name = terms[5]
    edt_guid = terms[6].strip('(),')
    energy = int(terms[7])

    key = edt_guid
    if not events.has_key(key):
        events[key] = dict()
    if event_type == 'start':
        events[key]['start_time'] = time
    else:
        events[key]['end_time'] = time
    events[key]['edt'] = edt_name
    component = cores[core_guid]

    events[key]['chip'] = component[0]
    events[key]['unit'] = component[1]
    events[key]['block'] = component[2]
    events[key]['xe'] = component[3]
    if not events[key].has_key('energy'): events[key]['energy'] = 0
    events[key]['energy'] += energy

def parse_db_line(terms):
    #time = terms[0]
    #event_type = terms[2].strip(':')
    #edt_name = terms[3]
    edt_guid = terms[4].strip('()')
    #db_guid = terms[6]
    #size = terms[7].strip('(),')
    energy = int(terms[8])

    key = edt_guid
    if not events.has_key(key):
        events[key] = dict()
        events[key]['energy'] = 0
    events[key]['energy'] += energy

def parse_file(filename, outfile):
    print 'Reading file...'
    with open(filename) as f:
        lines = [line.strip() for line in f]
    for line in lines:
        terms = line.split(' ');
        if 'min_time' not in locals():
            min_time = int(terms[0])
        else:
            min_time = min(min_time, int(terms[0]))

        if (terms[1] == "EDT"):
            parse_edt_line(terms)
        elif (terms[1] == "DB"):
            parse_db_line(terms)
    # Build database
    print 'Creating database...'
    conn = sqlite3.connect(outfile)
    c = conn.cursor()
    c.execute("DROP TABLE IF EXISTS %s" % EDT_TABLE_NAME)
    c.execute('''CREATE TABLE %s(
                 id INTEGER PRIMARY KEY,
                 chip INTEGER,
                 unit INTEGER,
                 block INTEGER,
                 xe INTEGER,
                 edt TEXT,
                 start_time INTEGER,
                 end_time INTEGER,
                 energy REAL,
                 power REAL)''' % EDT_TABLE_NAME)
    insert_string = '''INSERT INTO %s(id,chip,unit,block,xe,edt,start_time,end_time,energy,power) VALUES(%%s,%%s,%%s,%%s,%%s,'%%s',%%s,%%s,%%s,%%s)''' % EDT_TABLE_NAME
    for event in events.items():
        key = int(event[0], 16)
        value = event[1]
        try:
            s = insert_string % (key, value['chip'], value['unit'], value['block'],
                                 value['xe'], value['edt'], value['start_time']-min_time,
                                 value['end_time']-min_time, value['energy'], value['energy']/(value['end_time']-value['start_time']))
        except ZeroDivisionError:
            sys.stderr.write('Warning: Zero-length EDT instance found. Setting power to 0.\n')
            s = insert_string % (key, value['chip'], value['unit'], value['block'],
                                 value['xe'], value['edt'], value['start_time']-min_time,
                                 value['end_time']-min_time, value['energy'], 0)
        c.execute(s)
    # Insert some indexes to speed things up
    c.execute('CREATE INDEX chipIndex ON edts(chip)')
    c.execute('CREATE INDEX unitIndex ON edts(unit)')
    c.execute('CREATE INDEX blockIndex ON edts(block)')
    c.execute('CREATE INDEX xeIndex ON edts(xe)')
    c.execute('CREATE INDEX startIndex ON edts(start_time)')
    c.execute('CREATE INDEX endIndex ON edts(end_time)')

    # Add a second table for use with time data
    c.execute("DROP TABLE IF EXISTS %s" % TIME_TABLE_NAME)
    c.execute('''CREATE TABLE %s(
                 chip INTEGER,
                 unit INTEGER,
                 block INTEGER,
                 xe INTEGER,
                 time INTEGER PRIMARY KEY,
                 power REAL)''' % TIME_TABLE_NAME)
    select_string = '''SELECT chip,unit,block,xe,time,SUM(powerDelta) AS power FROM
                       (SELECT chip,unit,block,xe,start_time AS time,SUM(power) AS powerDelta FROM %s GROUP BY time
                        UNION
                        SELECT chip,unit,block,xe,end_time AS time,-SUM(power) AS powerDelta FROM %s GROUP BY time)
                       GROUP BY time''' % (EDT_TABLE_NAME, EDT_TABLE_NAME)
    insert_string = 'INSERT INTO %s ' % TIME_TABLE_NAME + select_string
    c.execute(insert_string)
    conn.commit()
    c.close()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('logfile', metavar='file', help='The logfile to parse into a database')
    parser.add_argument('--core-map', help='A file mapping core GUIDs to XEs. Defaults to cores.map', default=None)
    parser.add_argument('-o', '--outfile', help='The output sqlite database file. Defaults to %(default)s', default='out.db')
    args = vars(parser.parse_args())


    if args['core_map'] is None:
        print 'Generating core map...'
        gen_core_map(args['logfile'])
    else:
        print 'Reading core map...'
        read_core_map(args['core_map'])
    parse_file(args['logfile'], args['outfile'])

    #test_output()

def test_output():
    ''' Show an example '''
    k = events.keys()[0]
    e = events[k]
    print 'key '+k
    print 'start '+e['start_time']
    print 'end '+e['end_time']
    print 'edt '+e['edt']
    print 'core '+e['chip']+'.'+e['unit']+'.'+e['block']+'.'+e['xe']
    print 'energy '+e['energy']

if __name__ == "__main__":
    sys.exit(main())
