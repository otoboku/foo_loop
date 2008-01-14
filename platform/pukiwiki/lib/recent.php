<?php
/**
 * `RecentChanges' DB handling object
 *
 * remaining as `recent.dat'
 *
 * @since 2008-01-01
 */
class Pkwk_Recent {
  /**
   * max num of recent entry
   * @var int
   */
  var $conf = array();
  /**
   * recent entries
   * @var array
   */
  var $entries = array();
  /**
   * search pagename
   * @var string
   */
  var $search_page;

  /**
   * constructor
   *
   * priority of configuration
   * <pre>
   * 1. parameter for constructor
   * 2. ini file
   * 3. hard code internally
   * </pre>
   *
   * @since 2008-01-02
   * @param array $conf
   */
  function Pkwk_Recent( $conf = null ) {
    $this->_init_conf();
    $this->_load_conf();
    $this->_set_conf( $conf );
    $this->load();
  }

  /**
   * Initialize default configuration
   *
   * @since  2008-01-02
   * @return void
   * @access private
   */
  function _init_conf() {
    global $rss_max;

    $max_entry =  ( isset( $rss_max ) ) ? $rss_max : 30;
    if ( defined( 'CACHE_DIR' ) ) {
      $path = CACHE_DIR.'recent.dat';
    } else {
      $path = dirname( __FILE__ ).'/recent.dat';
    }
    $this->conf = array( 'max_entry' => $max_entry,
                         'path'      => $path,
                         'debug'     => false,
                         );
  }

  /**
   * load inifile
   *
   * @since  2008-01-02
   * @return boolean
   * @access private
   */
  function _load_conf() {
    $ret = false;

    $inifile = dirname( __FILE__ ).'/'.( basename( __FILE__, '.php' ).'.ini' );
    if ( file_exists( $inifile ) and is_readable( $inifile ) ) {
      $conf = parse_ini_file( $inifile );
      $ret = ( $this->_set_conf( $conf ) > 0 ) ? true : false;
    }

    return $ret;
  }

  /**
   * set configuration after initialize
   *
   * @since  2008-01-02
   * @param  array $conf
   * @return int
   * @access private
   */
  function _set_conf( $conf = null ) {
    $cnt = 0;

    if ( is_array( $conf ) and sizeof( $conf ) > 0 ) {
      foreach ( $this->conf as $item => $val ) {
        if ( array_key_exists( $item, $conf ) ) {
          $this->conf[$item] = $conf[$item];
          $cnt++;
        }
      }
    }

    return $cnt;
  }

  /**
   * path of DB
   *
   * @since  2008-01-02
   * @return mixed string or null
   */
  function path() {
    if ( is_array( $this->conf ) and
         array_key_exists( 'path', $this->conf ) ) {
      return $this->conf['path'];
    } else {
      return null;
    }
  }

  /**
   * read DB
   *
   * open file and return trimmed array
   *
   * @since  2008-01-03
   * @return array
   */
  function read() {
    $content = array();

    if ( $path = $this->path() and file_exists( $path ) and
         $fh = fopen( $path, 'rb' ) and flock( $fh, LOCK_SH ) ) {
      $content = array_map( 'trim', file( $path ) );
      flock( $fh, LOCK_UN );
      fclose( $fh );
    }

    return $content;
  }

  /**
   * clear DB
   *
   * @since  2008-01-03
   * @return void
   */
  function _clear() {
    $this->entries = array();
  }

  /**
   * parse given content lines' array and return complex array as content of DB
   *
   * @since  2008-01-01
   * @parse  array   $content
   * @param  boolean $limit
   * @return array
   */
  function parse( $content, $limit = false ) {
    if ( $limit ) {
      $content = array_slice( $content,
                              0,
                              $this->conf['max_entry'] );
    }
    $this->_clear();
    $this->entries = array_map( array( $this, '_split' ), $content );
    
    return $this->entries;
  }

  /**
   * read content, and cache it on memory, and return
   *
   * @since  2008-01-03
   * @param  boolean $limit
   * @return array
   */
  function load( $limit = false ) {
    return ( $this->entries = $this->parse( $this->read(), $limit ) );
  }

  /**
   * conver line into array
   *
   * @since  2008-01-03
   * @param  string $entry
   * @return array
   * @access private
   */
  function _split( $entry ) {
    $arr = explode( "\t", $entry );
    if ( sizeof( $arr ) > 1 ) {
      return array( 'unixtime' => $arr[0],
                    'pagename' => $arr[1] );
    } else {
      return null;
    }
  }

  /**
   * search page and create new array
   *
   * @since  2008-01-03
   * @param  string $pagename
   * @return array
   */
  function search( $pagename ) {
    $this->search_page = $pagename;
    return array_filter( $this->entries, array( $this, '_search_page' ) );
  }

  /**
   * callback function for looking for entry by pagename
   *
   * depend on $this->search_page
   *
   * @since  2008-01-03
   * @param  array $entry
   * @return boolean
   * @access private
   */
  function _search_page( $entry ) {
    if ( is_array( $entry ) and array_key_exists( 'pagename', $entry ) and
         $this->search_page == $entry['pagename'] ) {
      return true;
    } else {
      return false;
    }
  }

  /**
   * whether page exist or not
   *
   * @since  2008-01-03
   * @param  string $pagename
   * @return boolean
   */
  function exists( $pagename ) {
    return ( sizeof( $this->search( $pagename ) ) > 0 ) ? true : false;
  }

  /**
   * update entry
   *
   * @since  2008-01-03
   * @param  string  $pagename
   * @param  boolean $sync write immediately
   * @return boolean
   */
  function update( $pagename, $sync = true ) {
    if ( $this->exists( $pagename ) ) {
      $this->remove( $pagename, false );
    }
    $now   = time();
    array_unshift( $this->entries, array( 'unixtime' => (string)$now,
                                          'pagename' => $pagename )
                   );
    if ( $sync and !$this->conf['debug'] ) {
      $this->write();
    }
  }

  /**
   * remove entry 
   *
   * @since  2008-01-03
   * @param  string  $pagename
   * @param  boolean $sync write immediately
   * @return boolean
   */
  function remove( $pagename, $sync = true ) {
    $ret = false;

    $exists = $this->search( $pagename );
    if ( sizeof( $exists ) > 0 ) {
      unset( $this->entries[key( $exists )] );
      $this->entries = array_values( $this->entries );
      $ret = true;
      if ( $sync and !$this->conf['debug'] ) {
        $this->write();
      }
    }

    return $ret;
  }
  
  /**
   * sync db to file
   *
   * @since  2008-01-03
   * @return boolean
   */
  function write() {
    $ret = false;

    if ( $path = $this->path() ) {
      if ( !file_exists( $path ) and !touch( $path ) ) {
        return false;
      }
      if ( $fh = fopen( $path, 'r+b' ) and flock( $fh, LOCK_EX ) ) {
        $content = array_map( array( $this, '_serialize' ), $this->entries );
        ftruncate( $fh, 0 );
        fwrite( $fh, implode( "\n", $content )."\n" );
        flock( $fh, LOCK_UN );
        fclose( $fh );
        $ret = true;
      } 
    }

    return $ret;
  }

  /**
   * callback function for serializing a entry to one string
   *
   * @since  2008-01-03
   * @param  array $entry
   * @return string
   * @access private
   */
  function _serialize( $entry ) {
    if ( is_array( $entry ) and array_key_exists( 'unixtime', $entry ) and
         array_key_exists( 'pagename', $entry ) ) {
      return sprintf( "%s\t%s", $entry['unixtime'], $entry['pagename'] );
    } else {
      return '';
    }
  }
} // End of class Pkwk_Recent
