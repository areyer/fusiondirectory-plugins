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

class faiManagement extends management
{
  // Tab definition
  protected $tabClass = "";
  protected $tabType = "";
  protected $aclCategory = "";
  protected $aclPlugin   = "";
  protected $objectName   = "FAI object";

  //  Attributes Managed by this plugin can be used in post events
  public $attributes   = array("lock_type","lock_name","lock_dn");

  var $dispNewBranch=false;
  var $dispNewFreeze=false;

  var $fai_release = ""; // The currently selected release while in release management mode!
  var $fai_base = "";
  var $acl_base = "";

  var $lock_type = "";
  var $lock_dn = "";
  var $lock_name = "";


  function __construct($config,$ui)
  {
    $this->config = $config;

    $this->fai_base = get_ou("faiBaseRDN").$this->config->current['BASE'];
    /* FIXME */
    $cfg_rel = $this->config->search("faiManagement","DEFAULTFAIRELEASE",array("menu"));
    if (!empty($cfg_rel)) {
      $this->fai_release = $cfg_rel;
    } else {
      $this->fai_release = $this->fai_base;
    }

    $releases = $this->getReleaseList();

    if (!isset($releases[$this->fai_release])) {
      $this->fai_release = key($releases);
    }

    $this->acl_base = $this->config->current['BASE'];
    $this->ui = $ui;
    $this->storagePoints = array(
        get_ou('faiPartitionRDN'),
        get_ou('faiPackageRDN'),
        get_ou('faiScriptRDN'),
        get_ou('faiVariableRDN'),
        get_ou('faiHookRDN'),
        get_ou('faiProfileRDN'),
        get_ou('faiTemplateRDN'));

    // Build filter
    if (session::global_is_set(get_class($this)."_filter")) {
      $filter= session::global_get(get_class($this)."_filter");
    } else {
      $filter = new filter(get_template_path("fai-filter.xml", true));
      $filter->setObjectStorage($this->storagePoints);
    }
    $filter->elementValues['RELEASE'] = $this->fai_release;
    $this->setFilter($filter);

    // Build headpage
    $headpage = new listing(get_template_path("fai-list.xml", true));
    $headpage->setFilter($filter);
    $headpage->setBase($this->fai_release);
    $headpage->registerElementFilter("filterProperties", "faiManagement::filterProperties");
    $headpage->registerElementFilter("objectTypes", "listing::filterObjectTypes");

    $this->registerAction("remove_multiple","removeEntryRequested");
    $this->registerAction("new_profile","newEntry");
    $this->registerAction("new_template","newEntry");
    $this->registerAction("new_script","newEntry");
    $this->registerAction("new_hook","newEntry");
    $this->registerAction("new_variable","newEntry");
    $this->registerAction("new_package","newEntry");
    $this->registerAction("new_partition","newEntry");
    $this->registerAction("newClassNameSelected","newClassNameSelected");
    $this->registerAction("editByGroup","editByGroup");
    $this->registerAction("createBranch","createBranch");
    $this->registerAction("createFreeze","createFreeze");
    $this->registerAction("removeBranch","removeBranch");
    $this->registerAction("removeBranchConfirmed","removeBranchConfirmed");
    $this->registerAction("saveBranch","saveBranch");
    $this->registerAction("PerformBranch","PerformBranch");

    parent::__construct($config, $ui, "FAI object", $headpage);
  }


  /*! \brief  Act on copy & paste actions here.
   */
  function copyPasteHandler($action="",$target=array(),$all=array(),$altTabClass ="",$altTabType="",$altAclCategory="",$altAclPlugin="")
  {
    // Collect real dns, the listed objects are grouped by their cn
    $headpage = $this->getHeadpage();
    if ($action == "copy") {

      if (count($target) == 1) {

        // We just want to copy a single FAI object, let the user choose entries from the FAI-Group
        $entry = $headpage->getEntry($target[0]);
        if (count($entry['GROUPS']) == 1) {
          $data = array_pop($entry['GROUPS']);
          $type = $this->get_type($data);
          $this->cpHandler->add_to_queue($data['dn'],"copy",$type[0],$type[2],'fai',$this);
          @DEBUG (DEBUG_TRACE, __LINE__, __FUNCTION__, __FILE__,$data['dn'],"Entry copied!");
        } else {
          $this->dialogObject = new faiGroupHandle($entry['GROUPS'],"copy");
        }

      } else {

        // More than one group was selected, expect that the user wants to copy the complete groups.
        foreach($target as $t) {
          $entry = $headpage->getEntry($t);

          // Check for valid FAI objects
          if (in_array('FAKE_OC_FAI', $entry['objectClass'])) {
            foreach($entry['GROUPS'] as $g) {
              $type = $this->get_type($g);
              $this->cpHandler->add_to_queue($g['dn'],"copy",$type[0],$type[2],'fai',$this);
              @DEBUG (DEBUG_TRACE, __LINE__, __FUNCTION__, __FILE__,$g['dn'],"Entry copied!");
            }
          }
        }
      }
    }

    // Initiate pasting
    if ($action == "paste") {
      $this->cpPastingStarted = TRUE;
    }

    // Display any c&p dialogs, eg. object modifications required before pasting.
    if ($this->cpPastingStarted && $this->cpHandler->entries_queued()) {
      $data = $this->cpHandler->execute($headpage->getBase());
      FAI::save_release_changes_now();
      if (!empty($data)) {
        return $data;
      }
    }

    // Automatically disable pasting process since there is no entry left to paste.
    if (isset($this->cpHandler) && !$this->cpHandler->entries_queued()) {
      $this->cpPastingStarted = FALSE;
    }
    return "";
  }


  /*! \brief  A new FAI object was requested, let the user specify a name theis object now.
   */
  function newEntry($action = "", $target = array(), $all = array(), $altTabClass = "", $altTabType = "", $altAclCategory = "")
  {
    $types = array(
        "new_partition"     =>  "FAIpartitionTable",
        "new_script"        =>  "FAIscript",
        "new_hook"          =>  "FAIhook",
        "new_variable"      =>  "FAIvariable",
        "new_template"      =>  "FAItemplate",
        "new_package"       =>  "FAIpackageList");
    $types_i18n = array(
        "new_partition"     =>  _("partition table"),
        "new_script"        =>  _("script"),
        "new_hook"          =>  _("hook"),
        "new_variable"      =>  _("variable"),
        "new_template"      =>  _("template"),
        "new_package"       =>  _("package list"));

    if (isset($types[$action])) {
      $type_acl_mapping = array(
          "FAIpartitionTable"  => "faiPartitionTable",
          "FAIpackageList"     => "faiPackage",
          "FAIscript"          => "faiScript",
          "FAIvariable"        => "faiVariable",
          "FAIhook"            => "faiHook",
          "FAIprofile"         => "faiProfile",
          "FAItemplate"        => "faiTemplate");

      $acl = $this->ui->get_permissions($this->acl_base, "fai/".$type_acl_mapping[$types[$action]]);
      if (preg_match("/c/", $acl)) {
        $this->dialogObject = new askClassName($this->config, $this->dn, $this->ui, $types[$action]);
        $this->dialogObject->parent = &$this;
      } else {
        msg_dialog::display(_("Permission error"),
            sprintf(_("You have no permission to create a new %s!"), $types_i18n[$action]), ERROR_DIALOG);
      }
    }
    if ($action == "new_profile") {
      $this->dn = "new";

      $acl = $this->ui->get_permissions($this->acl_base, "fai/faiProfile");
      if (preg_match("/c/", $acl)) {
        $type = $this->get_type(array("objectClass" => array("FAIprofile")));
        $str  = management::newEntry('newEntry', array(), array(), $type[0], $type[2], $type[1]);
        if ($str) {
          return $str;
        }
        $this->tabObject->set_acl_base($this->acl_base);
      } else {
        msg_dialog::display(_("Permission error"), sprintf(_("You have no permission to create a new %s!"), _("profile")), ERROR_DIALOG);
      }
    }
  }


  /*! \brief   A new FAI class was requested and the user had a specify a name for it.
   *           Here we check if this name is useable and then open the edit dialogs.
   */
  function newClassNameSelected()
  {
    $this->dialogObject->save_object();
    if (count($this->dialogObject->check()) != 0) {
      foreach ($this->dialogObject->check() as $msg) {
        msg_dialog::display(_("Error"), $msg, ERROR_DIALOG);
      }
    } elseif (isset($this->dialogObject->objectClass)) {
      $this->dn = "new";
      $type = $this->get_type(array("objectClass" => array($this->dialogObject->objectClass)));
      $name = $this->dialogObject->save();

      if (class_exists($type[0])) {
        $this->closeDialogs();
        $str = management::newEntry('newEntry', array(), array(), $type[0], $type[2], $type[1]);
        if ($str) {
          return $str;
        }
        $this->tabObject->set_acl_base($this->acl_base);
        $this->tabObject->by_object[$type[1]]->cn = $name;
      }
    }
  }


  /*! \brief   Edit the selected entry.
   *           If there was a FAI group clicked, display a dialog with all members of the group.
   */
  function editEntry($action="",$target=array(),$all=array(), $altTabClass ="", $altTabType = "", $altAclCategory="")
  {
    $headpage = $this->getHeadpage();
    if (count($target) == 1) {
      $entry = $headpage->getEntry($target[0]);
      if ($entry) {

          // Edit FAI objects here
          if (count($entry['GROUPS']) == 1) {
            $data = array_pop($entry['GROUPS']);
            $type = $this->get_type($data);
            $str = management::editEntry('editEntry',array($data['dn']),array(),$type[0],$type[2],$type[1]);
            if ($str) return($str);
            $this->tabObject->by_object[$type[1]]->FAIstate = $data['FAIstate'];
            $this->tabObject->read_only = preg_match("/freeze/i", $data['FAIstate']);

          } else {
            $this->dialogObject = new faiGroupHandle($entry['GROUPS'],"edit");
          }
        }
      }
    }


  /*! \brief   Someone wants to remove some object(s)
   *            ask for a confirmation now.
   */
  function removeEntryRequested($action="",$target=array(),$all=array(), $altTabClass ="", $altTabType = "", $altAclCategory="")
  {
    $this->closeDialogs();
    // Collect objects to delete
    $headpage = $this->getHeadpage();
    $to_delete = array();
    foreach ($target as $id) {
      $object = $headpage->getEntry($id);
      if (in_array("FAKE_OC_FAI", $object['objectClass'])) {
        foreach ($object['GROUPS']  as $entry) {
          array_push($to_delete, $entry);
        }
      }
    }
    return $this->removeFAIObjects($to_delete);
  }


  /*! \brief   Someone wants to remove some object(s)
   *            ask for a confirmation now.
   */
  function removeFAIObjects($to_delete)
  {
    // Do not allow to remove objects with state freeezed
    $errors = $disallowed = array();

    foreach($to_delete as $obj) {
      $type = $this->get_type($obj);
      $acl  = $this->ui->get_permissions($obj['dn'],"fai/".$type[1]);
      if(!preg_match("/d/",$acl)) {
        $disallowed[] = $obj['dn'];
      } elseif(isset($obj['FAIstate']) && preg_match('/^freeze/', $obj['FAIstate'])) {
        $errors[] = $obj['dn'];
      } else {
        $this->dns[] = $obj['dn'];
      }
    }
    if (count($errors)) {
      msg_dialog::display(_("Branch locked"),
          sprintf(_("The following entries are locked, you can't remove them %s."),msgPool::buildList($errors)),INFO_DIALOG);
    }
    if (count($disallowed)) {
      msg_dialog::display(_("Permission error"), msgPool::permDelete($disallowed), ERROR_DIALOG);
    }

    // Check entry locking
    if (count($this->dns)) {
      if ($user = get_multiple_locks($this->dns)) {
        return gen_locked_message($user,$this->dns);
      }
      $smarty = get_smarty();
      $dns_names = array();
      foreach ($this->dns as $dn) {
        add_lock ($dn, $this->ui->dn);
        $dns_names[] = LDAP::fix($dn);
      }
      $smarty->assign("info",msgPool::deleteInfo($dns_names, _("FAI object")));
      $smarty->assign("multiple", TRUE);
      return $smarty->fetch(get_template_path('remove.tpl', TRUE));
    }
  }


  /*! \brief   Entry removal is confirmed, now remove objects
   */
  function removeEntryConfirmed($action="",$target=array(),$all=array(),
      $altTabClass="",$altTabType="",$altAclCategory="")
  {
    $ldap = $this->config->get_ldap_link();
    $ldap->cd($this->config->current['BASE']);

    $disallowed = array();
    foreach($this->dns as $key => $dn){
      $ldap->cat($dn);
      if($ldap->count()){
        $attrs = $ldap->fetch();
        $type= $this->get_type($attrs);

        $acl  = $this->ui->get_permissions($dn,"fai/".$type[1]);
        if(preg_match("/d/",$acl)){

          // Now save changes
          $str = management::removeEntryConfirmed($action,array($dn),$all,$type[0],$type[2],$type[1]);
          if(!empty($str)) return($str);
          FAI::save_release_changes_now();
          $to_del = FAI::clean_up_releases($dn);
          foreach($to_del as $dn){
            $ldap->rmdir_recursive($dn);
          }

        } else {
          $disallowed[] = $dn;
          new log("security","fai/".get_class($this),$dn,array(),"Tried to trick deletion.");
        }
      }
    }

    /* Normally this shouldn't be reached, send some extra
       logs to notify the administrator */
    if(count($disallowed)){
      msg_dialog::display(_("Permission error"), msgPool::permDelete($disallowed), ERROR_DIALOG);
    }
  }


  /*! \brief   Someone clicked on edit/remove for a grouped FAI object.
   *           We are now going to display a dialog to let the user select the entry
   *            he wants to perform the action on.
   */
  function editByGroup()
  {
    if($this->dialogObject instanceOf faiGroupHandle && $this->dialogObject->get_mode() == "edit"){
      $this->dialogObject->save_object();
      $entry = $this->dialogObject->get_selected();
      $this->closeDialogs();
      $data = array_pop($entry);
      $type = $this->get_type($data);
      $str = management::editEntry('editEntry',array($data['dn']),array(),$type[0],$type[2],$type[1]);
      if($str) return($str);
      $this->tabObject->by_object[$type[1]]->FAIstate = $data['FAIstate'];
      $this->tabObject->read_only = preg_match("/freeze/i", $data['FAIstate']);
    }elseif($this->dialogObject instanceOf faiGroupHandle && $this->dialogObject->get_mode() == "remove"){
      $this->dialogObject->save_object();
      $to_delete = $entry = $this->dialogObject->get_selected();
      if(count($to_delete)) $this->closeDialogs();
      return($this->removeFAIObjects($to_delete));
    }elseif($this->dialogObject instanceOf faiGroupHandle && $this->dialogObject->get_mode() == "copy"){
      $this->dialogObject->save_object();
      $entries = $entry = $this->dialogObject->get_selected();
      if(count($entries)){
        foreach($entries as $entry){
          $type = $this->get_type($entry);
          $this->cpHandler->add_to_queue($entry['dn'],"copy",$type[0],$type[2],'fai',$this);
          @DEBUG (DEBUG_TRACE, __LINE__, __FUNCTION__, __FILE__,$entry['dn'],"Entry copied!");
        }
        $this->closeDialogs();
      }
    }
  }


  /*! \brief   Save dialog/object modifications
   */
  protected function saveChanges()
  {
    $str = management::saveChanges();
    if($str) return($str);

    // Now save changes
    FAI::save_release_changes_now();
    $to_del = FAI::clean_up_releases($this->last_dn);
    $ldap= $this->config->get_ldap_link();
    foreach($to_del as $dn){
      $ldap->rmdir_recursive($dn);
    }
  }


  /*! \brief   Save dialog/object modifications but keep the dialogs opened
   */
  protected function applyChanges()
  {
    $str = management::applyChanges();
    if($str) return($str);

    // Now save changes
    FAI::save_release_changes_now();
    $to_del = FAI::clean_up_releases($this->last_dn);
    foreach($to_del as $dn){
      $ldap->rmdir_recursive($dn);
    }
  }


  /*! \brief   Initiates release removal
   */
  function removeBranch()
  {
    /* Check if we have a post remove method configured
     *  else skip this operation. (Skip:Button in the ui should be disabled in this case too)
     */
    if ("" != $this->config->search("faiManagement", "POSTREMOVE", array('menu','tabs','hooks'))) {
      /* Load permissions for selected 'dn' and check if
         we're allowed to remove this 'dn' */
      if (preg_match("/d/",$this->ui->get_permissions($this->acl_base,"fai/faiManagement"))) {
        $smarty=get_smarty();
        $smarty->assign("release_hidden",base64_encode($this->fai_release));
        $smarty->assign("info", msgPool::deleteInfo(LDAP::fix($this->fai_release),_("FAI branch/freeze")));
        return $smarty->fetch(get_template_path('remove_branch.tpl',TRUE));
      } else {
        msg_dialog::display(_("Permission error"), _("You have no permission to delete this release!"), ERROR_DIALOG);
      }
    }
  }


  /*! \brief   Remove a release after removal was confirmed
   */
  function removeBranchConfirmed()
  {
    /* Check if we have a post remove method configured
     *  else skip this operation. (Skip:Button in the ui should be disabled in this case too)
     */
    if ("" != $this->config->search("faiManagement", "POSTREMOVE", array('menu','tabs','hooks'))) {

      if (!isset($_POST['release_hidden']) || base64_decode($_POST['release_hidden']) != $this->fai_release) {
        msg_dialog::display(_("Warning"),_("Release remove aborted because the release name check failed!"));
      } else {

        $bb =  $this->fai_release;
        $ldap = $this->config->get_ldap_link();

        $br = $this->getBranches();

        if(isset($br[$bb]) && preg_match("/d/",$this->ui->get_permissions($this->acl_base,"fai/faiManagement"))){
          $name = $br[$bb];

          $ldap->cd($bb);
          $ldap->recursive_remove();
          $ldap->cd(preg_replace('/,'.preg_quote(get_ou('faiBaseRDN'), '/').'/i', ','.get_ou('applicationRDN'), $bb));
          $ldap->recursive_remove();
          $ldap->cd(preg_replace('/,'.preg_quote(get_ou('faiBaseRDN'), '/').'/i', ','.get_ou('mimetypeRDN'), $bb));
          $ldap->recursive_remove();

          /* Search for all groups with configured application menus.
             - First search all groups, to ensure that we only remove entries form whithin groups.
             - The search für menu configuration for the specified release and collect all those dns.
             - Remove entries
           */
          $release_ou = preg_replace("/".preg_quote(get_ou("faiBaseRDN"), '/').".*$/i","",$bb);
          $ldap->cd($this->config->current['BASE']);
          $ldap->search("(objectClass=posixGroup)",array("dn"));

          /* Collect all group dns
           */
          $groups = array();
          while($attrs = $ldap->fetch()){
            $groups[] = $attrs['dn'];
          }


          /* Collect all group menu release dns that match the release we have removed
           */
          $dns = array();
          foreach($groups as $dn){
            $ldap->cd($dn);
            $ldap->search("(objectClass=FAIbranch)",array("dn"));
            while($attrs = $ldap->fetch()){
              if(preg_match("/^".preg_quote($release_ou, '/')."/",$attrs['dn'])){
                $dns[] = $attrs['dn'];
              }
            }
          }

          /* Finally remove collected release dns
           */
          foreach($dns as $dn){
            $ldap->cd($dn);
            $ldap->recursive_remove();
          }

          /* Post remove */
          $this->fai_release = $this->fai_base;
          $this->lock_name   = $name;
          $this->lock_dn     = $bb;
          $this->handle_post_events("remove");

          $fai_filter = session::get("fai_filter");
          $fai_filter['fai_release'] = $this->fai_release;
          session::set("fai_filter",$fai_filter);

          new log("remove","fai/".get_class($this),$br[$bb],array(),"Release removed");
        }
      }
    }
  }


  /*! \brief   Initiates release creation
   */
  function createBranch()
  {
    if ($this->config->search("faiManagement", "POSTCREATE",array('menu','tabs','hooks')) == "") {
      msg_dialog::display(_("Configuration"), msgPool::cmdnotfound("POSTCREATE", get_class()), ERROR_DIALOG);
    } elseif (!preg_match("/c/", $this->ui->get_permissions($this->acl_base, "fai/faiManagement"))) {
      msg_dialog::display(_("Permission error"), msgPool::permCreate(_("Branch")), ERROR_DIALOG);
    } else {
      $smarty = get_smarty();
      $this->dispNewBranch  = TRUE;
      $this->dispNewFreeze  = FALSE;
      $smarty->assign("iframe", FALSE);
      if (isset($_POST['BranchName'])) {
        $smarty->assign("BranchName", $_POST['BranchName']);
      } else {
        $smarty->assign("BranchName", "");
      }
      return $smarty->fetch(get_template_path('faiNewBranch.tpl', TRUE, dirname(__FILE__)));
    }
  }


  /*! \brief   Initiates release creation
   */
  function createFreeze()
  {
    if($this->config->search("faiManagement", "POSTCREATE",array('menu','tabs')) == ""){
      msg_dialog::display(_("Configuration"), msgPool::cmdnotfound("POSTCREATE", get_class()), ERROR_DIALOG);
    }elseif(!preg_match("/c/",$this->ui->get_permissions($this->acl_base,"fai/faiManagement"))){
      msg_dialog::display(_("Permission error"), msgPool::permCreate(_("Branch")), ERROR_DIALOG);
    }else{
      $smarty = get_smarty();
      $this->dispNewFreeze=true;
      $this->dispNewBranch=false;
      $smarty->assign("iframe",false);
      if(isset($_POST['BranchName'])){
        $smarty->assign("BranchName", $_POST['BranchName']);
      }else{
        $smarty->assign("BranchName","");
      }
      return($smarty->fetch(get_template_path('faiNewBranch.tpl', TRUE, dirname(__FILE__))));
    }
  }


  /*! \brief   Creates a new branch
   */
  function PerformBranch()
  {
    if(!preg_match("/c/",$this->ui->get_permissions($this->acl_base,"fai/faiManagement"))){
      msg_dialog::display(_("Permission error"), msgPool::permCreate(_("Branch")), ERROR_DIALOG);
    }else{

      /* In order to see error messages we have to reset the error handler.
         Due to the exit();
       */
      restore_error_handler();

      $this->dispNewBranch = false;
      $this->dispNewFreeze = false;

      $LASTPOST = session::get('LASTPOST');
      $base = $LASTPOST['base'];
      $_POST  = session::get('LASTPOST');
      $name = $_POST['BranchName'];

      $type = $LASTPOST['type'];
      $ldap = $this->config->get_ldap_link();

      $baseToUse = $base;
      if($this->fai_release !=  $this->fai_base){
        $baseToUse = $this->fai_release;
      }

      /* Create new Release name to be able to set faidebianRelease for FAIpackageList */

      $CurrentReleases  = $this->getBranches();
      $NewReleaseName   = $name;
      if(isset($CurrentReleases[$this->fai_release])) {
        if($this->fai_release != $this->fai_base){
          $NewReleaseName = $CurrentReleases[$this->fai_release]."/".$name;
          $NewReleaseName = preg_replace("#\/#","/",$NewReleaseName);
        }else{
          $NewReleaseName   = $name;
        }
      }
      $appsrc = preg_replace("/".preg_quote(get_ou('faiBaseRDN'), '/')."/i",get_ou('applicationRDN'),$baseToUse);
      $appdst = preg_replace("/".preg_quote(get_ou('faiBaseRDN'), '/')."/i",get_ou('applicationRDN'),"ou=".$name.",".$baseToUse) ;

      $mimesrc = preg_replace("/".preg_quote(get_ou('faiBaseRDN'), '/')."/i",get_ou('mimetypeRDN'),$baseToUse);
      $mimedst = preg_replace("/".preg_quote(get_ou('faiBaseRDN'), '/')."/i",get_ou('mimetypeRDN'),"ou=".$name.",".$baseToUse) ;

      /* Check if source depeartments exist */
      foreach(array($baseToUse,$appsrc,$mimesrc) as $dep){
        $ldap->cd($this->config->current['BASE']);
        $ldap->cat($dep);
        if(!$ldap->count()){
          $ldap->create_missing_trees($dep);
        }
      }

      /* Print header to have styles included */
      echo '  <!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 3.2//EN">
        <html>
        <head>
        <title></title>
        <style type="text/css">@import url("themes/default/style.css");</style>
        <script src="include/focus.js" type="text/javascript"></script>
        </head>
        <body style="background: none;margin:3px;color:black">
        ';

      new log("create","fai/".get_class($this),$NewReleaseName,array(),"New $type created");

      /* Duplicate group application releases
       */
      FAI::copy_FAI_group_releases($CurrentReleases[$this->fai_release],$name,$type);

      /* Duplicate applications
       */
      $ldap->cat($appsrc,array("dn")) ;
      if($ldap->count()){
        $ldap->cd ($appdst);
        $ldap->recursive_remove();
        FAI::copy_FAI_resource_recursive($appsrc,$appdst,$NewReleaseName,$type,true);
      }

      /* Duplicate mime types
       */
      $ldap->cat($mimesrc,array("dn")) ;
      if($ldap->count()){
        $ldap->cd ($mimedst);
        $ldap->recursive_remove();
        FAI::copy_FAI_resource_recursive($mimesrc,$mimedst,$NewReleaseName,$type,true);
      }

      $attr = array();
      $attr['objectClass'] = array("organizationalUnit","FAIbranch");
      $attr['ou']       = $name;
      $attr['FAIstate'] = $type;
      $ldap->cd($this->config->current['BASE']);
      $ldap->cd("ou=".$name.",".$baseToUse);
      $ldap->cat("ou=".$name.",".$baseToUse);
      if($ldap->count()){
        $ldap->modify($attr);
      }else{
        $ldap->add($attr);
      }

      /* Duplicate fai objects
       */
      //      $ldap->cd ("ou=".$name.",".$baseToUse);
      //      $ldap->recursive_remove();
      //      FAI::copy_FAI_resource_recursive($baseToUse,"ou=".$name.",".$baseToUse,$NewReleaseName,$type,true);

      echo "<div style='width:100%;text-align:right;'><form name='form' method='post' action='?plug=".$_GET['plug']."' target='_parent'>
        <br><input type='submit' name='CloseIFrame' value='"._("Continue")."'>
        <input type='hidden' name='php_c_check' value='1'>
        </form></div>";

      echo "<script language=\"javascript\" type=\"text/javascript\">scrollDown2();</script>" ;

      /* Print footer to have valid html */
      echo "</body></html>";

      $this->dispNewFreeze = false;

      /* Postcreate */

      /* Assign possible attributes */
      $this->lock_type  = $type;
      $this->lock_name  = $name;
      $this->lock_dn    = $baseToUse;
      $this->handle_post_events("add");

      exit();
    }
  }


  /*! \brief   Creates a new branch, after a useable name was specified.
   */
  function saveBranch()
  {
    if($this->dispNewBranch){
      $type = "branch";
    }else{
      $type = "freeze";
    }

    /* Check branch name */
    $name = $_POST['BranchName'];
    $is_ok = true;
    $smarty = get_smarty();
    $smarty->assign("BranchName",$name);
    $base= $this->fai_base;

    /* Check used characters */
    if(!preg_match("/^[0-9a-z\.]*$/",$name)){
      msg_dialog::display(_("Error"), msgPool::invalid(_("Name"),$name,"/[0-9a-z\.]/"), ERROR_DIALOG);
      $is_ok = false;
    }

    // Check if this name is already in use
    if(!$this->CheckNewBranchName($_POST['BranchName'],$this->fai_release)){
      msg_dialog::display(_("Error"), msgPool::duplicated(_("Name")), ERROR_DIALOG);
      $is_ok = false;
    }

    // Handle errors
    if(!$is_ok && $this->dispNewFreeze){
      return($this->createFreeze());
    }elseif(!$is_ok && $this->dispNewBranch){
      return($this->createBranch());
    }

    // Now create new release

    if(session::is_set('LASTPOST')){
      $LASTPOST = session::get('LASTPOST');
    }else{
      $LASTPOST = array();
    }
    $LASTPOST['base'] = $base;
    $LASTPOST['type'] = $type;
    $LASTPOST['BranchName'] = $name;
    session::set('LASTPOST',$LASTPOST);
    $smarty->assign("iframe", true);
    $smarty->assign("plugID", $_GET['plug']);
    $display  = $smarty->fetch(get_template_path('faiNewBranch.tpl', TRUE, dirname(__FILE__)));
    return($display);

  }



  /*! \brief   Returns a list of all releases for useable for drop down boxes.
   *                      ou=fai... /
   *              ou=siga,ou=fai... &nbsp; siga
   *         ou=1,ou=siga,ou=fai... &nbsp;&nbsp; 1
   */
  function getReleaseList($base = "", $prefix ="")
  {
    $list = array();
    if (empty($base)) {
      $base = $this->fai_base;
    }

    $ldap = $this->config->get_ldap_link();
    $ldap->ls("(objectClass=FAIbranch)",$base,array("ou","FAIstate"));

    while($release = $ldap->fetch()){
      $list[$release['dn']] = $prefix.$release['ou'][0];
      $list = array_merge($list,$this->getReleaseList($release['dn'],$prefix."&nbsp; "));
    }
    return $list;
  }


  /*! \brief   Returns a list of all releases with full release names
   *                      ou=fai... /
   *              ou=siga,ou=fai... siga
   *         ou=1,ou=siga,ou=fai... siga/1
   */
  function getBranches($base = false,$prefix = "")
  {
    $ret = array();
    $ldap = $this->config->get_ldap_link();
    if(!$base){
      $base = $this->fai_base;
    }
    $tmp = FAI::get_all_releases_from_base($base,true);
    foreach($tmp as $dn => $name){
      $ret[$name]=$dn;
    }
    ksort($ret);
    $ret = array_flip($ret);

    return ($ret);
  }


  /*! \brief   Detects object info like corresponding  tab,class,acl
   *    e.g.    [0]   =  tabsPartition
   *            [1]   =  faiPartitionTable
   *            [2]   =  FAIPARTITIONTABS
   */
  function get_type($array)
  {
    if (!isset($array['objectClass'])) return array();
    if (in_array("FAIpartitionTable",$array['objectClass'])) {
      return array("simpleTabs_noSpecial","faiPartitionTable","FAIPARTITIONTABS");
    }
    if (in_array("FAIscript",$array['objectClass'])) {
      return array("simpleTabs_noSpecial","faiScript","FAISCRIPTTABS");
    }
    if (in_array("FAItemplate",$array['objectClass'])) {
      return array("simpleTabs_noSpecial","faiTemplate","FAITEMPLATETABS");
    }
    if (in_array("FAIhook",$array['objectClass'])) {
      return array("simpleTabs_noSpecial","faiHook","FAIHOOKTABS");
    }
    if (in_array("FAIvariable",$array['objectClass'])) {
      return array("simpleTabs_noSpecial","faiVariable","FAIVARIABLETABS");
    }
    if (in_array("FAIprofile",$array['objectClass'])) {
      return array("simpleTabs_noSpecial","faiProfile","FAIPROFILETABS");
    }
    if (in_array("FAIpackageList",$array['objectClass'])) {
      return array("simpleTabs_noSpecial","faiPackage","FAIPACKAGETABS");
    }
    return array();
  }


  /*! \brief   Checks if the given string can be used as class name
   */
  static function check_class_name($oc,$name,$dn)
  {
    $base = FAI::get_release_dn($dn);

    if($oc == "FAIprofile"){
      $f = "";
      $ocs = array("FAIprofile","FAItemplate","FAIhook","FAIpartitionTable","FAIpackageList","FAIscript","FAIvariable");
      foreach($ocs as $oc){
        $f .= "(objectClass=".$oc.")";
      }
      $res  = FAI::get_all_objects_for_given_base($base,"(|".$f.")",TRUE);
    }else{
      $res  = FAI::get_all_objects_for_given_base($base,"(|(objectClass=FAIprofile)(objectClass=".$oc."))",TRUE);
    }
    $delete = array();
    $used   = array();
    foreach($res as $object){
      $used[$object['cn'][0]]= $object['cn'][0];
    }
    return($used);
  }


  /*! \brief   Checks if the given string can be used for a new release
   */
  function CheckNewBranchName($name,$base)
  {
    $f = $this->fai_release;
    if($name == ""){
      return(false);
    }elseif(in_array($name,$this->getBranches($f))) {
      return(false);
    }elseif(tests::is_department_name_reserved($name,$base)){
      return(false);
    }
    return(true);
  }


  /*! \brief   This filter is used to display small icons for each listed object
   *            instead of their typ names
   */
  static function filterProperties($row, $classes)
  {
    $objects = array(
        "FAIpartitionTable"  => array("IMG"=> "plugins/fai/images/fai_partitionTable.png",
          "NAME"=>_("Partition table"),"KZL"=> "PT", "VAR"=>"ShowPartitions"),
        "FAIpackageList"     => array("IMG"=> "plugins/fai/images/fai_packages.png",
          "NAME"=>_("Package list") ,  "KZL"=> "PL", "VAR"=>"ShowPackages"),
        "FAIscript"          => array("IMG"=> "plugins/fai/images/fai_script.png",
          "NAME"=>_("Scripts") ,       "KZL"=> "S",  "VAR"=>"ShowScripts"),
        "FAIvariable"        => array("IMG"=> "plugins/fai/images/fai_variable.png",
          "NAME"=>_("Variables") ,     "KZL"=> "V",  "VAR"=>"ShowVariables"),
        "FAIhook"            => array("IMG"=> "plugins/fai/images/fai_hook.png",
          "NAME"=>_("Hooks"),          "KZL"=> "H",  "VAR"=>"ShowHooks"),
        "FAIprofile"         => array("IMG"=> "plugins/fai/images/fai_profile.png",
          "NAME"=>_("Profile") ,       "KZL"=> "P",  "VAR"=>"ShowProfiles"),
        "FAItemplate"        => array("IMG"=> "plugins/fai/images/fai_template.png",
          "NAME"=>_("Templates") ,     "KZL"=> "T",  "VAR"=>"ShowTemplates"));

    $icon_list = "";
    foreach($objects as $type => $type_data){
      if(in_array($type, $classes)){
        $icon_list .= "<input type='image' src='".$type_data['IMG']."' title='".$type_data['NAME']."'
          alt='".$type_data['KZL']."' class='center' name='edit_".$row."_".$type."'>\n";
      } else {
        $icon_list .= "<img src='images/empty.png' alt=' ' class='center'>\n";
      }
    }

    return $icon_list;
  }


  /*! \brief   Overridden render method of class mangement.
   *            this allows us to add a release selection box.
   */
  function renderList()
  {
    $filter = $this->getFilter();
    $headpage = $this->getHeadpage();
    $filter->setComboBoxOptions("RELEASE",$this->getReleaseList());

    if(isset($_POST['RELEASE'])){
      $this->fai_release = get_post('RELEASE');
    }
    $headpage->setBase($this->fai_release);
    $headpage->update();
    $smarty = get_smarty();
    $smarty->assign("fai_release", $this->fai_release);
    $smarty->assign("fai_base", $this->fai_base);
    $r = $this->config->search("faiManagement", "POSTREMOVE",array('menu','tabs','hooks'));
    $c = $this->config->search("faiManagement", "POSTCREATE",array('menu','tabs','hooks'));
    $smarty->assign("allow_create", $c);
    $smarty->assign("allow_remove", $r);
    $display = $headpage->render();
    return($this->getHeader().$display);
  }


  /*! \brief   Convert POST and GET variables into actions.
   */
  function detectPostActions()
  {
    $action = management::detectPostActions();
    if(isset($_POST['remove_multiple'])) $action['action'] = "remove";
    if(isset($_POST['new_profile'])) $action['action'] = "new_profile";
    if(isset($_POST['new_template'])) $action['action'] = "new_template";
    if(isset($_POST['new_script'])) $action['action'] = "new_script";
    if(isset($_POST['new_hook'])) $action['action'] = "new_hook";
    if(isset($_POST['new_variable'])) $action['action'] = "new_variable";
    if(isset($_POST['new_package'])) $action['action'] = "new_package";
    if(isset($_POST['new_partition'])) $action['action'] = "new_partition";

    if(isset($_POST['save_properties'])) $action['action'] = "saveOpsiProperties";
    if(isset($_POST['cancel_properties'])) $action['action'] = "cancel";

    if(isset($_POST['edit_continue'])) $action['action'] = "newClassNameSelected";
    if(isset($_POST['edit_cancel'])) $action['action'] = "cancel";

    if(isset($_POST['faiGroupHandle_cancel'])) $action['action'] = "cancel";
    if(isset($_POST['CancelBranchName'])) $action['action'] = "cancel";
    if(isset($_POST['delete_branch_confirm'])) $action['action'] = "removeBranchConfirmed";
    if(isset($_GET['PerformBranch'])) $action['action'] = "PerformBranch";
    if(isset($_POST['UseBranchName'])) $action['action'] = "saveBranch";
    if(isset($_POST['faiGroupHandle_apply']))  $action['action'] = "editByGroup";
    if(isset($_GET['act']) && $_GET['act'] == "branch_branch")  $action['action'] = "createBranch";
    if(isset($_GET['act']) && $_GET['act'] == "freeze_branch")  $action['action'] = "createFreeze";
    if(isset($_GET['act']) && $_GET['act'] == "remove_branch")  $action['action'] = "removeBranch";

    foreach($_POST as $name => $value){
      if(preg_match("/^edit_([0-9]*)_([a-z_]*)_(x|y)/i", $name)){
        $id = preg_replace("/^edit_([0-9]*)_([a-z_]*)_(x|y)/i","\\1", $name);
        $tab = preg_replace("/^edit_([0-9]*)_([a-z_]*)_(x|y)/i","\\2", $name);

        $headpage = $this->getHeadpage();
        $entry = $headpage->entries[$id];

        if(in_array('FAKE_OC_FAI', $entry['objectClass'])){
          if(isset($headpage->entries[$id]['GROUPS'][$tab])){
            $data =$headpage->entries[$id]['GROUPS'][$tab];
            $type = $this->get_type($data);
            $str = management::editEntry('editEntry',array($data['dn']),array(),$type[0],$type[2],$type[1]);
            if($str) return($str);
          }
        }else{
          $str = $this->editEntry('editEntry',array($entry['dn']));
          if($str) return($str);
        }
        break;
      }
    }
    return($action);
  }


  static function plInfo()
  {
    return array(
      "plShortName"   => _("FAI"),
      "plDescription" => _("Manage FAI software packages and deployment recipes"),
      "plIcon"        => "plugins/fai/images/icon.png",
      "plSelfModify"  => FALSE,
      "plSection"     => "admin",
      "plPriority"    => 30,
      "plCategory"    => array("fai" => array("description" => _("FAI"), "objectClass" => "FAIclass")),

      "plProvidedAcls" => array()
    );
  }
}
?>
