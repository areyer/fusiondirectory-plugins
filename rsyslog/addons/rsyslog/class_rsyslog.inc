<?php
/*
  This code is part of FusionDirectory (http://www.fusiondirectory.org/)
  Copyright (C) 2003-2010  Cajus Pollmeier
  Copyright (C) 2011-2013  FusionDirectory

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
*/
require_once 'MDB2.php';

class rsyslog extends plugin
{
  var $servers            = array();
  var $selected_server    = "";
  var $selected_host      = "all";
  var $selected_priority  = "all";

  var $startTime = "";
  var $stopTime  = "";

  var $search_for = "";

  var $sort_value = "DeviceReportedTime";
  var $sort_type  = "DESC";
  var $limit      = "0";
  var $limits     = array(20,50,100,200,500,1000,'-');
  var $page       = 0;

  /*! \brief  Return plugin informations for acl handling
      @return Array ACL infos of this plugin.
   */
  static function plInfo()
  {
    return array(
      "plShortName"   => _("System logs"),
      "plDescription" => _("This plugin allow to View system logs"),
      "plSelfModify"  => FALSE,
      "plSection"     => "addons",
      "plPriority"    => 3,
      "plCategory"    => array("server/serviceSyslog"),
      "plObjectType"  => array("rsyslog" => array("name" => _("System logs"))),

      "plProvidedAcls" => array()
    );
  }

  function __construct (&$config, $dn = NULL)
  {
    $this->config = &$config;
    $this->ui     = get_userinfo();

    $this->startTime = date("d.m.Y", (time() - 7 * 24 * 60 * 60));
    $this->stopTime  = date("d.m.Y", time());

    // Get list of rsyslog servers
    $ldap = $this->config->get_ldap_link();
    $ldap->cd($this->config->current['BASE']);
    $ldap->search('objectClass=goLogDBServer', array('cn','goLogDriver','goLogDB','goLogAdmin','goLogPassword'));
    while ($attrs = $ldap->fetch()) {
      if (empty($this->selected_server)) {
        $this->selected_server = $attrs['cn'][0];
      }
      $s = array(
        'cn'          => $attrs['cn'][0],
        'dn'          => $attrs['dn'],
        'DRIVER'      => $attrs['goLogDriver'][0],
        'DB'          => $attrs['goLogDB'][0],
        'LOGIN'       => $attrs['goLogAdmin'][0],
        'PASSWORD'    => $attrs['goLogPassword'][0],
        'SERVER'      => $attrs['cn'][0],
        'Hosts'       => array(),
        'Priorities'  => array(),
        'ACL'         => '',
        'status'      => 'ok'
      );

      $this->servers[$attrs['cn'][0]] = $s;
    }

    // Test server connectivity
    $ui = get_userinfo();
    foreach ($this->servers as $name => $server) {
      $link = databaseManagement::connectDatabase($server);
      if (PEAR::isError($link)) {
        new log("debug", "gosa_logging", "dbconnect", array(), $link->getMessage());
        $this->servers[$name]['status'] = "connectfailed";
        $this->servers[$name]['error']  = $link->getMessage();
        continue;
      } else {
        $this->servers[$name]['status'] = "ok";
        $this->servers[$name]['error']  = "";

        // Detect ACLs
        $this->servers[$name]['ACL'] = $ui->get_permissions($server['dn'], 'server/rsyslog', 'viewEntries');

        // Get list of Hosts using this log server..
        if (preg_match('/r/', $this->servers[$name]['ACL'])) {
          $query = "SELECT distinct(FromHost) FROM SystemEvents;";
          $res = $link->query($query);
          if (PEAR::isError($res)) {
            $this->servers[$name]['status'] = "query_failed";
            $this->servers[$name]['error']  = $res->getMessage();
          } else {
            while ($attrs = $res->fetchRow(MDB2_FETCHMODE_ASSOC)) {
              $this->servers[$name]['Hosts'][$attrs['fromhost']] = $attrs['fromhost'];
            }
          }
          $query = "SELECT distinct(Priority) FROM SystemEvents ORDER BY Priority;";
          $res = $link->query($query);
          if (PEAR::isError($res)) {
            $this->servers[$name]['status'] = "query_failed";
            $this->servers[$name]['error']  = $res->getMessage();
          } else {
            while ($attrs = $res->fetchRow(MDB2_FETCHMODE_ASSOC)) {
              $this->servers[$name]['Priorities'][$attrs['priority']] = $attrs['priority'];
            }
          }
        }
      }
    }
  }

  function execute()
  {
    $smarty = get_smarty();
    $smarty->assign("usePrototype", "true");
    $ui = get_userinfo();

    if (!isset($this->servers[$this->selected_server]['Hosts']) || !count($this->servers[$this->selected_server]['Hosts'])) {
      $hosts = array('-' => _("-"));
    } else {
      $hosts = array('all' => _("All"));
      $hosts = array_merge($hosts, $this->servers[$this->selected_server]['Hosts']);
    }
    if (!isset($this->servers[$this->selected_server]['Priorities']) || !count($this->servers[$this->selected_server]['Priorities'])) {
      $priorities = array('-' => _("-"));
    } else {
      $priorities = array('all' => _("All"));
      foreach ($this->servers[$this->selected_server]['Priorities'] as $id => $name) {
        $priorities[$id] = $this->severity2string($name);
      }
    }

    $result = $this->get_result();

    $smarty->assign("servers",            $this->servers);
    $smarty->assign("hosts",              $hosts);
    $smarty->assign("priorities",         $priorities);
    $smarty->assign("selected_server",    $this->selected_server);
    $smarty->assign("selected_host",      $this->selected_host);
    $smarty->assign("selected_priority",  $this->selected_priority);
    $smarty->assign("search_for", $this->search_for);
    $smarty->assign("startTime",  $this->startTime);
    $smarty->assign("stopTime",   $this->stopTime);
    $smarty->assign("sort_type",  $this->sort_type);
    $smarty->assign("sort_value", $this->sort_value);
    $smarty->assign("limits",     $this->limits);
    $smarty->assign("limit",      $this->limit);
    $smarty->assign("page",       $this->page);
    $smarty->assign("plug_id",  $_GET['plug']);
    $smarty->assign("downimg",  "<img src='images/lists/sort-down.png' class='center' alt='\\/' border='0'>");
    $smarty->assign("upimg",    "<img src='images/lists/sort-up.png' class='center' alt='\\/' border='0'>");
    $smarty->assign("result",   $result);
    $smarty->assign("matches",  sprintf(_("%s entries match the filter"), $result["count"]));
    if ($this->limits[$this->limit] != '-') {
      $smarty->assign("page_sel", range_selector($result['count'], $this->page, $this->limits[$this->limit]));
    } else {
      $smarty->assign("page_sel", "");
    }

    return $smarty->fetch(get_template_path("rSyslog.tpl", TRUE));
  }

  function get_result()
  {
    $result = array();
    $result['entries'] = array();
    $result['status']  = "ok";
    $result['count']   = 0;
    $result['error']   = "";

    // Check whether the selected server exists.
    if ($this->selected_server == "unknown" || !isset($this->servers[$this->selected_server])) {
      $this->servers["unknown"]['status'] = sprintf(_("The selected server '%s' does not exists!"), $this->selected_server);
      $this->servers["unknown"]["cn"]     = "-";
      return $result;
    }

    // Check database connectivity
    $name = $this->selected_server;

    $server = $this->servers[$name];

    $link = databaseManagement::connectDatabase($server);
    if (PEAR::isError($link)) {
      new log("debug", "gosa_logging", "dbconnect", array(), $link->getMessage());
      $this->servers[$name]['status'] = "connectfailed";
      $this->servers[$name]['error']  = $link->getMessage();
    } else {
      $this->servers[$name]['status'] = "ok";
      $this->servers[$name]['error']  = "";
    }

    // Get log entries matching the selected filter
    if (isset($this->servers[$name]['ACL']) && preg_match('/r/', $this->servers[$name]['ACL'])) {

      $host = "%%";
      if ($this->selected_host != "all") $host = $this->selected_host;
      $prio = "%%";
      if ($this->selected_priority != "all") $prio = $this->selected_priority;

      $filter = "FROM SystemEvents WHERE 1=1 ";
      if ($host != "%%") {
        $filter .= " AND FromHost like '{$host}' ";
      }
      if ($prio != "%%") {
        $filter .= "AND Priority like '{$prio}' ";
      }

      $filter .= "AND DeviceReportedTime >= '".(date("Ymd000000", strtotime($this->startTime)))."' ";
      $filter .= "AND DeviceReportedTime <= '".(date("Ymd235959", strtotime($this->stopTime)))."' ";

      if (!empty($this->search_for)) {
        $filter .= "AND ( ( SysLogTag like '%".$this->search_for."%' ) OR  ( Message like '%".$this->search_for."%' ) ) ";
      }

      // Detect amount of matching entries
      $query = "SELECT count(ID) as `amount` ".$filter;
      $res = $link->query($query);
      if (PEAR::isError($res)) {
        $this->servers[$name]['status'] = "query_failed";
        $this->servers[$name]['error']  = $res->getMessage();
      } elseif ($attrs = $res->fetchRow(MDB2_FETCHMODE_ASSOC)) {
        $result['count'] = $attrs['amount'];
      }

      if ($result['count']) {

        if ($result['count'] < $this->page) $this->page = 0;

        $filter .= "ORDER BY ".$this->sort_value." ".$this->sort_type.", DeviceReportedTime ".$this->sort_type." ";
        if ($this->limits[$this->limit] != "-") {
          $filter .= "LIMIT ".$this->page.", ".$this->limits[$this->limit];
        } else {
          $this->page = 0;
        }

        $query = "SELECT * ".$filter;
        $res = $link->query($query);
        if (PEAR::isError($res)) {
          $this->servers[$name]['status'] = "query_failed";
          $this->servers[$name]['error']  = $res->getMessage();
        } else {
          while ($attrs = $res->fetchRow(MDB2_FETCHMODE_ASSOC)) {
            $attrs['facility']  = $this->facility2string($attrs['facility']);
            $attrs['priority']  = $this->severity2string($attrs['priority']);
            $attrs['syslogtag'] = preg_replace("/:$/", "", $attrs['syslogtag']);

            foreach ($attrs as $key => $val) {
              $attrs[$key] = htmlentities($val, ENT_COMPAT, 'UTF-8');
            }

            $result['entries'][] = $attrs;
          }
          $this->servers[$name]['status'] = "ok";
          $this->servers[$name]['error']  = "";
        }
      }
    }
    $result['status'] = $this->servers[$name]['status'];
    $result['error']  = $this->servers[$name]['error'];
    return $result;
  }


  function save_object()
  {
    // Get timestamps
    foreach (array("stopTime","startTime") as $attr) {
      if (isset($_POST[$attr])) {
        $this->$attr = get_post($attr);
      }
    }

    // Get server from post
    if (isset($_POST['selected_server']) && isset($this->servers[$_POST['selected_server']])) {
      $this->selected_server = $_POST['selected_server'];
    }

    // Get Host
    if (isset($this->servers[$this->selected_server]['Hosts']) && isset($_POST['selected_host']) &&
        ($_POST['selected_host'] == "all" ||
         in_array($_POST['selected_host'], $this->servers[$this->selected_server]['Hosts']))) {
      $this->selected_host = $_POST['selected_host'];
    }

    // Get priority from post
    if (isset($this->servers[$this->selected_server]['Priorities']) && isset($_POST['selected_priority']) &&
        ($_POST['selected_priority'] == "all" ||
         in_array($_POST['selected_priority'], $this->servers[$this->selected_server]['Priorities']))) {
      $this->selected_priority = $_POST['selected_priority'];
    }

    // Get serach string
    if (isset($_POST['search_for'])) {
      $this->search_for = trim(get_post('search_for'));
    }

    // Get page navigation attributes
    if (isset($_GET['start'])) {
      $this->page = $_GET['start'];
    }
    if (isset($_POST['limit']) && isset($this->limits[$_POST['limit']])) {
      $this->limit = $_POST['limit'];
    }

    // Get sorting attributes
    if (isset($_GET['sort_value']) && in_array($_GET['sort_value'],
          array("DeviceReportedTime","FromHost","Facility","Priority","Message","SysLogTag"))) {
      $sort_value = $_GET['sort_value'];
      if ($this->sort_value == $sort_value) {
        if ($this->sort_type == "ASC") {
          $this->sort_type = "DESC";
        } else {
          $this->sort_type = "ASC";
        }
      }
      $this->sort_value = $sort_value;
    }
  }


  function severity2string($severity)
  {
    $map = array(
      0 => _("Emergency"),
      1 => _("Alert"),
      2 => _("Critical"),
      3 => _("Error"),
      4 => _("Warning"),
      5 => _("Notice"),
      6 => _("Informational"),
      7 => _("Debug")
    );

    if (isset($map[$severity])) {
      return $map[$severity];
    }

    return sprintf(_("Unknown (%s)"), $severity);
  }


  function facility2string($facility)
  {
    $map = array(
      0   => _("Kernel"),
      1   => _("User level"),
      2   => _("Mail system"),
      3   => _("System daemon"),
      4   => _("Security"),
      5   => _("Internal"),
      6   => _("Printing"),
      7   => _("News"),
      8   => _("UUCP"),
      9   => _("Cron"),
      10  => _("Security"),
      11  => _("FTP"),
      12  => _("NTP"),
      13  => _("Log audit"),
      14  => _("Log alert"),
      15  => _("Clock"),
      16  => "Local0",
      17  => "Local1",
      18  => "Local2",
      19  => "Local3",
      20  => "Local4",
      21  => "Local5",
      22  => "Local6",
      23  => "Local7"
    );

    if (isset($map[$facility])) {
      return $map[$facility];
    }

    return sprintf(_("Unknown (%s)"), $facility);
  }

  function time2local($str)
  {
    list($date, $time)  = explode(" ", $str);
    list($y, $m, $d)    = explode("-", $date);
    return "$d.$m.$y $time";
  }
}
?>
