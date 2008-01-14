<?php
/**
 * tests for recent.php
 *
 * require : simpletest testing framework from pearified.com channel
 *
 * @since 2008-01-02
 */
/** Unit Test Frame */
require_once( 'Pearified/Testing/SimpleTest/unit_tester.php' );
/** Test Result Reporter */
require_once( 'Pearified/Testing/SimpleTest/reporter.php' );

/** module that i want test */
require_once( dirname( __FILE__ ).'/../../lib/recent.php' );

class TestRecent extends UnitTestCase {
  var $obj;
  var $verbose   = false;
  var $exists    = 'SandBox';
  var $not_exist = 'foooooo';

  function TestRecent() {
  }

  function setUp() {
    copy( 'recent.dat.bak', 'recent.dat' );
    $this->obj = new Pkwk_Recent( array( 'path' =>
                                         dirname( __FILE__ ).'/recent.dat' ) );
  }

  function test_init_conf() {
    print "\n<h2><a href='#_init_conf'>Test of _init_conf()</a></h2>\n";
    print "<pre id='_init_conf'>";
    $this->obj->_init_conf();
    print "</pre>";
  }

  function test_load_conf() {
    print "\n<h2><a href='#_load_conf'>Test of _load_conf()</a></h2>\n";
    print "<pre id='_load_conf'>";
    $this->assertTrue( $result = $this->obj->_load_conf() );
    if ( !$result or $this->verbose ) {
      var_dump( $this->obj->conf );
    }
    print "</pre>";
  }

  function test_set_conf() {
    print "\n<h2><a href='#_set_conf'>Test of _set_conf()</a></h2>\n";
    print "<pre id='_set_conf'>";
    $this->assertFalse( $this->obj->_set_conf() );
    print "</pre>";
  }

  function test_path() {
    print "\n<h2><a href='#path'>Test of path()</a></h2>\n";
    print "<pre id='path'>";
    $this->assertEqual( realpath( $this->obj->path() ),
                        $this->obj->path() );
    print "</pre>";
  }

  function test_read() {
    print "\n<h2><a href='#read'>Test of read()</a></h2>\n";
    print "<pre id='read'>";
    if ( $this->verbose ) {
      var_dump( $this->obj->read() );
    }
    print "</pre>";
  }

  function test_clear() {
    print "\n<h2><a href='#_clear'>Test of _clear()</a></h2>\n";
    print "<pre id='_clear'>";
    $this->assertTrue( sizeof( $this->obj->entries ) > 0 );
    $this->obj->_clear();
    $this->assertTrue( sizeof( $this->obj->entries ) == 0 );
    print "</pre>";
  }

  function test_parse() {
    print "\n<h2><a href='parse'>Test of parse()</a></h2>\n";
    print "<pre id='parse'>";
    $result = $this->obj->parse( $this->obj->read() );
    if ( !$this->assertTrue( is_array( $result ) ) or
         $this->verbose ) {
      var_dump( $result );
    }
    print "</pre>";
  }

  function test_load() {
    print "\n<h2><a href='#load'>Test of load()</a></h2>\n";
    print "<pre id='load'>";
    if ( $this->verbose ) {
      var_dump( $this->obj->load() );
    }
    print "</pre>";
  }

  function test_split() {
    print "\n<h2><a href='#_split'>Test of _split()</a></h2>\n";
    print "<pre id='_split'>";
    $tests = array( 'ijdeiwoj'     => null,
                    'wjiwed edjei' => null,
                    'iejdi	weijdi' => array( 'unixtime' => 'iejdi',
                                                  'pagename' => 'weijdi' ),
                    'qwd	eid	wei' => array( 'unixtime' => 'qwd',
                                                       'pagename' => 'eid'  ),
                    );
    foreach ( $test as $arg => $result ) {
      $this->assertEqual( $this->obj->_split( $arg ), $result );
    }
    print "</pre>";
  }

  function test_search() {
    print "\n<h2><a href='#search'>Test of search()</a></h2>\n";
    print "<pre id='search'>";
    if ( $this->verbose ) {
      var_dump( $this->obj->search( $this->exists ) );
    }
    print "</pre>";
  }

  function test_search_page() {
    print "\n<h2><a href='#_search_page'>Test of _search_page()</a></h2>\n";
    print "<pre id='_search_page'>";
    $this->obj->search_page = $this->exists;
    $tests = array( array( array( 'pagename' => $this->exists ),
                           array( 'unixtime' => null ),
                           true),
                    array( array( 'pagename' => $this->exists ),
                           true),
                    array( array( 'pagename' => $this->not_exist ),
                           false ),
                    );
    foreach ( $tests as $test ) {
      if ( !$this->assertEqual( $this->obj->_search_page( $test[0] ),
                                $test[1] ) ) {
        var_dump( $test );
      }
    }
    print "</pre>";
  }

  function test_exists() {
    print "\n<h2><a href='#exists'>Test of exists()</a></h2>\n";
    print "<pre id='exists'>";
    $this->assertTrue( $this->obj->exists( $this->exists ) );
    $this->assertFalse( $this->obj->exists( $this->not_exist ) );
    print "</pre>";
  }

  function test_update() {
    print "\n<h2><a href='#update'>Test of update()</a></h2>\n";
    print "<pre id='update'>";
    $org     =  $this->obj->search( $this->exists );
    $org_val = current( $org );
    $this->obj->update( $this->exists, false );
    $new     = $this->obj->search( $this->exists );
    $new_key = key( $new );
    $new_val = current( $new );
    $this->assertTrue( $new_key == 0 );
    if ( $this->verbose ) {
      print "org ";
      var_dump( $org_val );
      print "new ";
      var_dump( $new_val );
    }
    $this->assertTrue( $new_val['unixtime'] > $org_val['unixtime'] );
    print "</pre>";
  }

  function test_remove() {
    print "\n<h2><a href='#remove'>Test of remove()</a></h2>\n";
    print "<pre id='remove'>";
    // remove
    $org     = $this->obj->read();
    $sandbox = array( $this->obj->_serialize( current( $this->obj->search( $this->exists ) ) ) );
    $this->assertTrue( $result = $this->obj->remove( $this->exists, false ) );
    $this->assertTrue( sizeof( $org ) > sizeof( $this->obj->entries ) );
    $this->assertFalse( $this->obj->remove( $this->not_exist, false ) );
    
    if ( !$result or $this->verbose ) {
      var_dump( $this->obj->entries );
    }
    // check
    $removed = array_map( array( $this->obj, '_serialize' ),
                          $this->obj->entries );
    if ( !$this->assertTrue( $sandbox == array_values( array_diff( $org, $removed ) ) ) ) {
      print "org ";
      var_dump( $org );
      print "removed ";
      var_dump( $removed );
    }
    print "</pre>";
  }

  function test_write() {
    print "\n<h2><a href='#write'>Test of write()</a></h2>\n";
    print "<pre id='write'>";
    // write indeed ?
    $mtime   = filemtime( $this->obj->path() );
    $content = serialize( $this->obj->load() );
    sleep( 1 );
    clearstatcache();
    $this->obj->write();
    $this->assertTrue( filemtime( $this->obj->path() ) > $mtime );
    // keep same content ?
    $this->assertTrue( $content == serialize( $this->obj->load() ) );
    // can create ?
    unlink( $this->obj->path() );
    $this->assertTrue( $this->obj->write() );
    print "</pre>";
  }
}

/**
 * Run tests
 */
if ( realpath( $_SERVER['SCRIPT_FILENAME'] ) == __FILE__ ) {
  $test =& new TestRecent();
  $html =& new HtmlReporter();
  $html->paintHeader( 'Test of lib/recent.php' );
  $test->run( $html );
}
