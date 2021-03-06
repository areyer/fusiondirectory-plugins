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

class faxreport extends plugin
{
  /* For internal use */
  var $start          = 0;
  var $search_for     = "*";
  var $search_base    = "";
  var $year           = "";
  var $month          = "";
  var $sort           = 1;
  var $sort_direction = "down";
  var $report_list    = array();
  var $ui             = NULL;
  var $range          = 20;
  var $view_logged = FALSE;

  /* Constant stuff */
  var $status= array( "SENT", "MAILED", "SERROR", "RERROR", "SBLOCK", "RBLOCK",
      "DELETED", "REQUEUED", "DISABLED", "PRINTED", "DIVERTED",
      "UNDEFINED", "UNDEFINED", "UNDEFINED", "UNDEFINED",
      "UNDEFINED");
  var $fields= array("uid", "queuing_time", "status", "sender_id", "receiver_id", "pages");

  /* these vars will be stored in session to be able to remember last search config */
  var $attributes_SO= array("search_for","search_base","month","year","start","year","month","sort","sort_direction","range");
  var $objectclasses= array();
  var $fax_users    = array();


  /* Create class */
  function faxreport (&$config, &$ui)
  {
    /* Include config object */
    $this->config       = $config;
    $this->ui           = &$ui;
    $this->search_base  = get_base_from_people($ui->dn);
    $this->year         = date("Y");
    $this->month        = date("m");

    /* Get global filter config and set class vars ,
       or create a filter */
    if (!session::is_set("faxreportfilter")){
      $faxreportfilter = array();
      foreach($this->attributes_SO as $name){
        $faxreportfilter[$name] = $this->$name;
      }
      session::set("faxreportfilter",$faxreportfilter);
    }else{
      $faxreportfilter = session::get("faxreportfilter");
      foreach($this->attributes_SO as $name){
        $this->$name = $faxreportfilter[$name];
      }
    }
  }

  /* Create Filter & Search & Display results */
  function execute()
  {
    /* Call parent execute */
    plugin::execute();

    /* Log view */
    if(!$this->view_logged){
      $this->view_logged = TRUE;
      new log("view","user/".get_class($this),$this->dn);
    }

    /************
      Variable initialisation
     ************/

    /* Create months */
    $months= array();
    for($i = 1 ; $i <= 12 ; $i ++ ){
      $months[$i] = _(date("F",gmmktime(0,0,0,$i,1)));
    }

    /* Create years */
    $current= date("Y");
    $years= array();
    for ($y= $current - 5; $y<=$current; $y++){
      $years[]= $y;
    }


    /************
      Set smarty defaults
     ************/

    $smarty= get_smarty();
    $smarty->assign("launchimage", get_template_path('images/launch.png'));
    $smarty->assign("search_image", 'geticon.php?context=actions&icon=system-search&size=16');
    $smarty->assign("search_for", $this->search_for);
    $smarty->assign("bases", $this->config->idepartments);
    $smarty->assign("base_select", $this->search_base);
    $smarty->assign("months", $months);
    $smarty->assign("month_select", $this->month);
    $smarty->assign("years", $years);
    $smarty->assign("year_select", $this->year);
    $smarty->assign("search_result", "");


    /************
      Check database accessibility
     ************/

    /* Some checks */
    if (!isset($this->config->data['SERVERS']['FAX'])) {
      msg_dialog::display(_("Error"), _("No fax server found!"), ERROR_DIALOG);
      return($smarty->fetch(get_template_path('contents.tpl', TRUE)));
    } else {
      /* Connecting, selecting database */
      $cfg = $this->config->data['SERVERS']['FAX'][0];// FIXME : should support multiple FAX servers
      $cfg['DB'] = "gofax";

      $link = databaseManagement::connectDatabase($cfg);
      if (PEAR::isError($link)) {
        msg_dialog::display(_("Error"), sprintf(_("Cannot connect to %s database!"), $cfg['DB']), ERROR_DIALOG);
        return($smarty->fetch(get_template_path('contents.tpl', TRUE)));
      }
    }


    /************
      Perform a detail view
     ************/

    /* Do detail view? */
    if (isset($_GET['detail'])) {

      /* Create query */
      $query = "SELECT id,uid,date_format(queuing_time, '%Y%m%d%H%i%s') as queuing_time,status,sender_id,sender_msn,receiver_id,".
        "receiver_msn,pages,status_message,transfer_time FROM faxlog WHERE id=".$_GET['detail'].";";

      /* Check if everything went ok */
      $result =& $link->query($query);
      if (PEAR::isError($result)) {
        msg_dialog::display(_("Error"), sprintf(_("Cannot query %s database!"), $cfg['DB']), ERROR_DIALOG);
        @DEBUG (DEBUG_DB, __LINE__, __FUNCTION__, __FILE__,$query, "Database query failed");
        return($smarty->fetch(get_template_path('contents.tpl', TRUE)));
      }

      $line = $result->fetchRow(MDB2_FETCHMODE_ASSOC);
      $link->disconnect();

      if (!preg_match ("/'".$line["uid"]."'/", $this->userfilter)) {
        msg_dialog::display(_("Permission error"), _("You have no permission to view this fax id!"), ERROR_DIALOG);
        return($smarty->fetch(get_template_path('contents.tpl', TRUE)));
      }

      /* Check acls */
      $fax_uids = array_flip($this->fax_users);
      $uid = $line['uid'];
      $dn = $fax_uids[$uid];
      $acls = $this->ui->get_permissions($dn,"faxreport/faxreport");
      if (!preg_match("/r/",$acls)) {
        msg_dialog::display(_("Permission error"), _("You have no permission to view this fax id!"), ERROR_DIALOG);
        return $smarty->fetch(get_template_path('contents.tpl', TRUE));
      }

      $parts = array( "id", "uid", "queuing_time", "status", "sender_id", "sender_msn",
          "receiver_id", "receiver_msn", "pages", "status_message", "transfer_time" );

      foreach ($parts as $vname) {

        $final="fax_$vname";
        if ($vname != "uid") {
          $v_acl = $this->ui->get_permissions($dn,"faxreport/faxreport",preg_replace("/_/","",$vname));
        } else {
          $v_acl = "r";
        }

        if ($line[$vname] != "" && preg_match("/r/",$v_acl)) {
          $smarty->assign("$final", $line[$vname]);
        } else {
          $smarty->assign("$final", "-");
        }
      }
      $queuing_time = $line['queuing_time'];

      /* The user is allowed to download all fax images from those users
          that are listed in session::get('fuserfilter')
         Don't forget to check getfax.php if you change somthing here */
      session::set('fuserfilter',$this->userfilter);
      $smarty->assign("plug", "?plug=".validate($_GET['plug']));
      $smarty->assign("detail", validate($_GET['detail']));

      $format = _("Y-M-D");
      $date = preg_replace("/Y/", substr($queuing_time,0,4), $format);
      $date = preg_replace("/M/", substr($queuing_time,4,2), $date);
      $date = preg_replace("/D/", substr($queuing_time,6,2), $date);
      $smarty->assign("date", $date);
      $smarty->assign("time", substr($queuing_time,8,2).":".
          substr($queuing_time,10,2).":".
          substr($queuing_time,12,2));
      return $smarty->fetch(get_template_path('detail.tpl', TRUE));
    }


    /************
      Search for uids matching the filter
     ************/

    /* Search button has been pressed */
    if ($this->search_for != "") {

      if (is_integer (strpos($this->search_for, "*"))) {
        $s= $this->search_for;
      } else {
        $s= "*".$this->search_for."*";
      }
      $ldap= $this->config->get_ldap_link();
      $ldap->cd($this->search_base);

      /* Perform ldap search for potential users */
      $filter = "(&(objectClass=gosaAccount)(!(objectClass=gosaUserTemplate))".
          "(objectClass=goFaxAccount)".
          "(|(uid=$s)(l=$s)(homePhone=$s)".
          "(telephoneNumber=$s)(facsimileTelephoneNumber=$s)(mobile=$s)".
          "(pager=$s)(cn=$s)(givenName=$s)(sn=$s)(personalTitle=$s)".
          "(title=$s)))";

      $res = get_list($filter, "user", $this->search_base, array("uid"), GL_SUBSEARCH );

      /* Check if we are allowed to collect fax data */
      $fax_users = array();
      foreach ($res as $attrs) {
        $acl = $this->ui->get_permissions($attrs['dn'],"user/user","uid");
        if (preg_match("/r/",$acl)) {
          $fax_users[ $attrs['dn']]= $attrs["uid"][0];
        }
      }

      /* Add self uid, if we have self acl
       */
      if(preg_match("/(r|w)/",$this->ui->get_permissions($this->ui->dn,"faxreport/faxreport",""))){
        $fax_users[$this->ui->dn] = $this->ui->uid;
      }

      $this->fax_users = $fax_users;

      /* Prepare SQL query */
      $this->userfilter= "";
      foreach ($fax_users as $user){
        $this->userfilter.= "uid = '$user' OR ";
      }
      $this->userfilter= preg_replace("/OR $/", "", $this->userfilter);
    }

    /************
      Create filter
     ************/

    /* Perform SQL query */
    if ($this->userfilter) {
      if ($this->sort_direction == "down") {
        $desc = "DESC";
      } else {
        $desc = "";
      }
      $start = date ("YmdHis", mktime(0,0,0,$this->month,1,$this->year));
      $end =   date ("YmdHis", mktime(23,59,59,$this->month+1,0,$this->year));
      $query = "SELECT id,uid,date_format(queuing_time, '%Y%m%d%H%i%s') as queuing_time,status,sender_id,receiver_id,pages FROM faxlog ".
        "WHERE ( ".$this->userfilter." ) AND queuing_time <= $end AND ".
        "queuing_time >= $start ORDER BY ".$this->fields[$this->sort]." $desc;";


    /************
      Create results
     ************/

      /* Connecting, selecting database */
      $cfg = $this->config->data['SERVERS']['FAX'][0];// FIXME : should support multiple FAX servers
      $link = databaseManagement::connectDatabase($cfg);

      @DEBUG (DEBUG_DB, __LINE__, __FUNCTION__, __FILE__,$query, "Database query");
      $result = $link->query($query);
      if (PEAR::isError($result)) {
        msg_dialog::display(_("Error"), sprintf(_("Cannot query %s database!"), $cfg['DB']), ERROR_DIALOG);
        return($smarty->fetch(get_template_path('contents.tpl', TRUE)));
      }

      $this->report_list = array();
      $user_ids = array_flip($fax_users);
      while ($line = $result->fetchRow(MDB2_FETCHMODE_ASSOC)) {

        /* Check permissions for each field */
        $dn = $user_ids[$line['uid']];

        /* Hide restricted attributes */
        foreach (array("pages","receiverid","senderid","status","queuingtime","detailedView") as $attr) {
          $var = $attr."ACL";
          $$var = $this->ui->get_permissions($dn,"faxreport/faxreport",$attr);
        }

        /* Restricted attributes will not be displayed, this will be displayed instead */
        $no_acl = "<img class='center' src='geticon.php?context=status&icon=object-locked&size=16'
                    title='"._("Insufficient permissions to view this attribute")."' alt='"._("Insufficient permissions")."'>";

        /* Create date */
        if ((!empty($line["queuing_time"])) && preg_match("/r/",$queuingtimeACL)) {
          $hour=  substr($line["queuing_time"], 8, 2);
          $minute=substr($line["queuing_time"], 10, 2);
          $format= _("Y-M-D");
          $date= preg_replace("/Y/", substr($line["queuing_time"], 0, 4), $format);
          $date= preg_replace("/M/", substr($line["queuing_time"], 4, 2), $date);
          $date= preg_replace("/D/", substr($line["queuing_time"], 6, 2), $date);
          $str_date = $date." ".$hour.":".$minute;
        } else {
          $str_date = $no_acl;
        }

        /* Create entry html str */

        if (preg_match("/r/",$detailedViewACL)) {
          $str = "<td class=\"phonelist\">
            <a href=\"main.php?plug=".validate($_GET['plug'])."&amp;detail=".$line["id"]."\">
            <img class='center' alt=\"\" align=\"middle\" border=0 src=\"".get_template_path('images/info_small.png')."\">
            &nbsp;".$line["uid"].
            "</a>
            </td>
            <td>$str_date</td>";
        } else {
          $str = "<td>
            <img class='center' alt=\"\" align=\"middle\" border=0 src=\"".get_template_path('images/info_small.png')."\">
            &nbsp;".$line["uid"]."</td>
            <td>$str_date</td>";
        }

        /* Add Status td */
        if (preg_match("/r/",$statusACL)) {
          $str.="<td>".$this->status[$line["status"]]."</td>";
        } else {
          $str.="<td>".$no_acl."</td>";
        }

        /* Add sender_id td */
        if (preg_match("/r/",$senderidACL)) {
          $str.="<td>".$line["sender_id"]."</td>";
        } else {
          $str.="<td>".$no_acl."</td>";
        }

        /* Add receiver_id td */
        if (preg_match("/r/",$receiveridACL)) {
          $str.="<td>".$line["receiver_id"]."</td>";
        } else {
          $str.="<td>".$no_acl."</td>";
        }

        /* Add receiver_id td */
        if (preg_match("/r/",$pagesACL)) {
          $str.="<td>".$line["pages"]."</td>";
        } else {
          $str.="<td>".$no_acl."</td>";
        }
      $this->report_list[] =  $str;
      }

      $link->disconnect();
    }

    /************
      Create output out of results
     ************/

    /* Generate output */
    $mod= 0;
    $output= "";
    foreach ($this->report_list as $val) {
      if ($mod < $this->start) {
        $mod++;
        continue;
      }
      if ($mod >= ($this->start + $this->range)) {
        $mod++;
        break;
      }
      if (($mod++) & 1) {
        $col= "background-color: #ECECEC;";
      } else {
        $col= "background-color: #F5F5F5;";
      }
      $output.= "<tr style=\"height:22px; $col\">$val</tr>";
    }


    /************
      Display results
     ************/

    if (isset($fax_users) && count($fax_users)) {
      $smarty->assign("search_result", $output);
      $smarty->assign("range_selector", range_selector(count($this->report_list), $this->start, $this->range,"EntriesPerPage"));
    } else {
      $smarty->assign("search_result", "");
    }

    /* Show main page */
    $smarty->assign("plug", "?plug=".validate($_GET['plug']));
    for ($i= 0; $i<7; $i++) {
      $smarty->assign("mode$i", "");
    }
    $smarty->assign("mode".$this->sort, "<img alt=\"\" src=\"images/sort_".$this->sort_direction.
        ".png\" border=0 align=middle>");
    return $smarty->fetch(get_template_path('contents.tpl', TRUE));
  }


  /* Save ui input, and store it in session::get()
     to remember last search next time*/
  function save_object()
  {
    $faxreportfilter = session::get("faxreportfilter");
    if (isset($_POST['EntriesPerPage'])) {
      $this->range = $_POST['EntriesPerPage'];
    }

    if (isset($_GET['start'])) {
      $this->start= (int)$_GET['start'];
    }

    /* Adapt sorting */
    if (isset($_GET['sort'])) {
      if ($this->sort == (int)$_GET['sort']) {
        if ($this->sort_direction == "down") {
          $this->sort_direction= "up";
        } else {
          $this->sort_direction= "down";
        }
      }
      $this->sort= (int)$_GET['sort'];
      if ($this->sort < 0 || $this->sort > 5) {
        $this->sort= 0;
      }
    }
    foreach ( array("year", "month", "search_for", "search_base") as $type) {
      if (isset($_POST[$type])) {
        $faxreportfilter[$type]= $_POST[$type];

        /* reset start page, if filter has changed */
        if (!isset($_GET['start'])) {
          $this->start = 0;
        }
      }
      $this->$type= $faxreportfilter[$type];

    }
    foreach ($this->attributes_SO as $name) {
      $faxreportfilter[$name] = $this->$name;
    }
    session::set("faxreportfilter",$faxreportfilter);
  }


  /* Return plugin informations for acl handling
    #FIXME You can only read attributes within this report plugin */
  static function plInfo()
  {
    return array(
      "plShortName"     => _("Fax reports"),
      "plDescription"   => _("Fax reports")."&nbsp;<i>"._("All entries are readonly")."</i>",
      "plIcon"          => "plugins/fax/images/iconReport.png",
      "plSelfModify"    => TRUE,
      "plSection"       => "addons",
      "plPriority"      => 5,
      "plCategory"      => array("faxreport" => array("description" => _("Fax reports"),
                                                     "objectClass" => array())),

      "plProvidedAcls" => array(
        "detailedView"   => _("Detailed view"),
        "id"             => _("Fax ID"),
        "queuingtime"    => _("Date")."&nbsp;/&nbsp;"._("Time"),
        "status"         => _("Status"),
        "senderid"       => _("Sender ID"),
        "sendermsn"      => _("Sender MSN"),
        "receiverid"     => _("Receiver ID"),
        "receivermsn"    => _("Receiver MSN"),
        "pages"          => _("Number of pages"),
        "statusmessage"  => _("Status Message"),
        "transfertime"   => _("Transfer time")
      )
    );
  }
}
?>
