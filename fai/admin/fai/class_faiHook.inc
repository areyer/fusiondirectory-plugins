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

class faiHook extends faiPluginClass
{
  /* attribute list for save action */
  var $ignore_account   = TRUE;

  /* Attributes for this Object */
  var $attributes       = array("cn","description");

  /* ObjectClasses for this Object*/
  var $objectclasses    = array("top","FAIclass","FAIhook");

  /* Class name of the Ldap ObjectClass for the Sub Object */
  var $subClass         = "FAIhookEntry";
  var $subClasses       = array("top","FAIclass","FAIhookEntry");

  /* Class name of the php class which allows us to edit a Sub Object */
  var $subClassName     = "faiHookEntry";

  /* Attributes to initialise for each subObject */
  var $subAttributes    = array("cn","description","FAItask");
  var $sub_Load_Later   = array("FAIscript");
  var $sub64coded       = array();
  var $subBinary        = array("FAIscript");

  /* Specific attributes */
  var $cn               = "";       // The class name for this object
  var $description      = "";       // The description for this set of partitions
  var $is_dialog        = false;    // specifies which buttons will be shown to save or abort
  var $SubObjects       = array();  // All leafobjects of this object
  var $view_logged      = FALSE;
  var $FAIstate         = "";
  var $ui;

  function faiHook (&$config, $dn= NULL)
  {
    /* Load Attributes */
    parent::__construct($config, $dn);

    /* If "dn==new" we try to create a new entry
     * Else we must read all objects from ldap which belong to this entry.
     */
    if($dn != "new"){
      $this->dn =$dn;

      /* Read all leaf objects of this object (For FAIscript this would be FAIscriptEntry)
       */
      $res = FAI::get_all_objects_for_given_base($this->dn,"(&(objectClass=FAIclass)(objectClass=".$this->subClass."))");
      foreach($res as $obj){

        /* Skip not relevant objects */
        if(!preg_match("/".preg_quote($this->dn, '/')."$/i",$obj['dn'])) continue;

        $objects = array();
        $objects['status']      = "FreshLoaded";
        $objects['dn']          = $obj['dn'];
        $objects                = $this->get_object_attributes($objects,$this->subAttributes);
        $this->SubObjects[$objects['cn']] = $objects;
      }
    }
    $this->is_new = FALSE;
    if($this->dn == "new"){
      $this->is_new =TRUE;
    }
    $this->ui = get_userinfo();
  }


  /* Reload some attributes */
  function get_object_attributes($object,$attributes)
  {
    $ldap = $this->config->get_ldap_link();
    $ldap->cd($this->config->current['BASE']);
    $ldap->cat($object['dn'],$attributes);
    $tmp  = $ldap->fetch();

    foreach($attributes as $attrs){
      if(isset($tmp[$attrs][0])){
        $var = $tmp[$attrs][0];

        /* Check if we must decode some attributes */
        if(in_array_ics($attrs,$this->sub64coded)){
          $var = base64_decode($var);
        }

        /*  check if this is a binary entry */
        if(in_array_ics($attrs,$this->subBinary)){
          $var = $ldap->get_attribute($object['dn'], $attrs,$r_array=0);
        }

        /* Fix slashes */
        $var = addslashes($var);
        $object[$attrs] = $var;
      }
    }
    return($object);
  }


  function acl_base_for_current_object($dn)
  {
    if($dn == "new" || $dn == ""){
      if($this->dn == "new"){
        $dn= $this->parent->parent->acl_base;
      }else{
        $dn = $this->dn;
      }
    }
    return($dn);
  }


  function getUsedFAItask($cn)
  {
    $ret = array();
    foreach($this->SubObjects as $name => $class){
      if($class['cn'] == $cn){
        continue;
      }
      if($class['status'] != "delete"){
        $ret[$class['FAItask']] = $class['FAItask'];
      }
    }
    return($ret);
  }

  function execute()
  {
    /* Call parent execute */
    parent::execute();

    if($this->is_account && !$this->view_logged){
      $this->view_logged = TRUE;
      new log("view","fai/".get_class($this),$this->dn);
    }

    /* Fill templating stuff */
    $smarty= get_smarty();
    $display= "";

    /* New Listhandling
     */
    $once = true;
    foreach($_POST as $name => $value){
      if(preg_match("/^editscript_/",$name)&&($once)){
        $once = false;
        $entry = preg_replace("/^editscript_/","",$name);
        $entry = base64_decode(preg_replace("/_.*/","",$entry));

        $obj  = $this->SubObjects[$entry];
        if($obj['status'] == "FreshLoaded"){
          $obj  = $this->get_object_attributes($obj,$this->sub_Load_Later);
        }

        $this->dialog= new $this->subClassName($this->config,$this->dn,$obj);
        $this->dialog->FAIstate = $this->FAIstate;
        $this->dialog->set_acl_base($this->acl_base_for_current_object($obj['dn']));
        $this->dialog->set_acl_category("fai");

        set_object_info($obj['dn']);
        $this->dialog->parent = &$this;
        $this->is_dialog=true;
      }
      if(preg_match("/^deletescript_/",$name)&&($once)){
        $entry = preg_replace("/^deletescript_/","",$name);
        $entry = base64_decode(preg_replace("/_.*/","",$entry));

        $dn = $this->acl_base_for_current_object($this->SubObjects[$entry]['dn']);
        $acl = $this->ui -> get_permissions($dn,"fai/faiScriptEntry")  ;

        if(preg_match("/d/",$acl)){
          $once = false;

          $status = $this->SubObjects[$entry]['status'];
          if($status == "edited" || $status == "FreshLoaded"){
            $this->SubObjects[$entry]['status']= "delete";
          }else{
            unset($this->SubObjects[$entry]);
          }
        }
      }
    }

    /* File download requested */
    if(isset($_GET['getFAIHook'])){
      if(isset($this->SubObjects[base64_decode($_GET['getFAIHook'])])){
        $obj = $this->SubObjects[base64_decode($_GET['getFAIHook'])];
        $obj  = $this->get_object_attributes($obj,$this->sub_Load_Later);
        send_binary_content(stripslashes($obj['FAIscript']),$obj['cn'].".FAIhook");
      }
    }

    /* Edit entries via GET */
    if(isset($_GET['act']) && isset($_GET['id'])){
      if($_GET['act'] == "edit" && isset($this->SubObjects[base64_decode($_GET['id'])])){
        $obj = $this->SubObjects[base64_decode($_GET['id'])];
        if($obj['status'] == "FreshLoaded"){
          $obj  = $this->get_object_attributes($obj,$this->sub_Load_Later);
        }
        $this->dialog= new $this->subClassName($this->config,$this->dn,$obj);
        $this->dialog->FAIstate = $this->FAIstate;
        $this->dialog->acl = $this->acl;
        $this->dialog->set_acl_base($this->acl_base_for_current_object($obj['dn']));
        $this->dialog->set_acl_category("fai");
        set_object_info($obj['dn']);
        $this->dialog->parent = &$this;
        $this->is_dialog=true;
      }
    }

    ///// Ende new list handling

    /* Add new sub object */
    if(isset($_POST['AddSubObject'])){

      $c_dn = $this->acl_base_for_current_object($this->dn);
      $this->dialog= new $this->subClassName($this->config,"new");
      $this->dialog->set_acl_base($c_dn);
      $this->dialog->FAIstate = $this->FAIstate;
      $this->dialog->set_acl_category("fai");
      $this->dialog->parent = &$this;
      $this->is_dialog=true;
    }


    /* Save Dialog */
    if(isset($_POST['SaveSubObject'])){

      /* Perform post check*/
      $this->dialog->save_object();

      /* Get messages */
      $msgs = $this->dialog->check();

      /* print errors */
      if(count($msgs)>0){
        foreach($msgs as $msg){
          msg_dialog::display(_("Error"), $msg, ERROR_DIALOG);
        }
      }else{

        /* Get return object */
        $obj = $this->dialog->save();
        if(isset($obj['remove'])){

          $old_stat = $this->SubObjects[$obj['remove']['from']]['status'];

          /* Depending on status, set new status */
          if($old_stat == "edited" || $old_stat == "FreshLoaded"){
            $this->SubObjects[$obj['remove']['from']]['status'] = "delete";
          }elseif($this->SubObjects[$obj['remove']['from']]['status']=="new"){
            unset($this->SubObjects[$obj['remove']['from']]);
          }
          $obj['status'] = "new";
          $this->SubObjects[$obj['remove']['to']] = $obj;
          unset($this->SubObjects[$obj['remove']['to']]['remove']);
        }else{
          if($obj['status'] == "FreshLoaded"){
            $obj['status'] = "edited";
          }
          $this->SubObjects[$obj['cn']]=$obj;
        }
        $this->is_dialog=false;
        unset($this->dialog);
        $this->dialog=FALSE;
      }
    }

    /* Sort entries */
    $tmp = $keys = array();
    foreach($this->SubObjects as $key => $entry){
      $keys[$key]=$key;
    }
    natcasesort($keys);
    foreach($keys as $key){
      $tmp[$key]=$this->SubObjects[$key];
    }
    $this->SubObjects = $tmp;

    /* Cancel Dialog */
    if(isset($_POST['CancelSubObject'])){
      $this->is_dialog=false;
      unset($this->dialog);
      $this->dialog=FALSE;
    }

    /* Print dialog if $this->dialog is set */
    if(is_object($this->dialog)){
      $this->dialog->save_object();
      $display = $this->dialog->execute();
      return($display);
    }



    /* Divlist            added 28.02.2006
       Containing FAIscripts
     */

    $divselectbox = new divSelectBox("FAIhooks");
    $divselectbox->setHeight(400);

    foreach($this->getList(true) as $key => $name){

      $dn= $this->acl_base_for_current_object($name['dn']);
      $acl = $this->ui->get_permissions($dn,"fai/faiHookEntry")  ;
      $act = "";

      /* Check if this object is freezed, in this case hide the delete icon */
      if(preg_match("/^freeze/", $this->FAIstate)){
        $act .= "<input type='image' src='geticon.php?context=actions&icon=document-edit&size=16'      name='editscript_%s'    title='"._("Edit")."' alt='"._("Edit")."'>";
      }else{
        $act .= "<input type='image' src='geticon.php?context=actions&icon=document-edit&size=16'      name='editscript_%s'    title='"._("Edit")."' alt='"._("Edit")."'>";
        if(preg_match("/d/",$acl)){
          $act .="<input type='image' src='geticon.php?context=actions&icon=edit-delete&size=16' name='deletescript_%s'  title='"._("Delete")."' alt='"._("Delete")."'>";
        }
      }

      /* Check if we are allowed to use the export button for this object */
      $s_acl = $this->ui->get_permissions($dn,"fai/faiHookEntry","FAIscript")  ;
      if(($this->SubObjects[$key]['status'] == "new") || ($this->SubObjects[$key]['dn'] == "new") || !preg_match("/r/",$s_acl)){
        $down = "";
      }else{
        $down = "<a href='?plug=".$_GET['plug']."&getFAIHook=".base64_encode($key)."'>
          <img src='images/save.png' alt='"._("Download")."' title='"._("Download")."' border=0>
          </a>";
      }

      /* Check if we are allowed to view the object */
      $s_acl = $this->ui->get_permissions($dn,"fai/faiHookEntry","cn")  ;
      if(preg_match("/r/",$s_acl)){

        $edit_link = "<a href='?plug=".$_GET['plug']."&amp;act=edit&amp;id=".base64_encode($key)."'>".$name['name']."</a>";
        $divselectbox->AddEntry(array(
          array("html"=>$edit_link),
          array("html"=>$down , "attach" => "style='width:20px;'"),
          array("html"=>str_replace("%s",base64_encode($key),$act),
                "attach"=>"style='border-right: 0px;width:50px;text-align:right;'")
        ));
      }
    }
    $smarty->assign("Entry_divlist",$divselectbox->DrawList());
    /* Divlist creation complete
     */

    $smarty->assign("SubObjects",$this->getList());

    foreach($this->attributes as $attrs){
      $smarty->assign($attrs,($this->$attrs));
    }

    $tmp = $this->plInfo();

    $c_dn = $this->acl_base_for_current_object($this->dn);
    $smarty->assign("sub_object_is_addable", preg_match("/c/",$this->ui->get_permissions($c_dn,"fai/faiHookEntry")) && !preg_match('/^freeze/', $this->FAIstate));
    foreach($tmp['plProvidedAcls'] as $name => $translation){
      $smarty->assign($name."ACL",$this->getacl($name));
    }

    $display.= $smarty->fetch(get_template_path('faiHook.tpl', TRUE));
    return($display);
  }

  /* Generate listbox friendly SubObject list
   */
  function getList($use_dns=false){
    $a_return=array();
    foreach($this->SubObjects as $obj){
      if($obj['status'] != "delete"){

        $cn   = stripslashes($obj['cn']);
        $desc = "";

        if((isset($obj['description']))&&(!empty($obj['description']))){
          $desc = " [".stripslashes($obj['description'])."]";
        }

        if($use_dns){
          $a_return[$obj['cn']]['name']= $cn.$desc;
          $a_return[$obj['cn']]['dn']= $obj['dn'];
        }else{
          $a_return[$obj['cn']] =  $cn.$desc;
        }
      }
    }
    return($a_return);
  }


  /* Delete me, and all my subtrees
   */
  function remove_from_parent()
  {
    if($this->acl_is_removeable()){
      $ldap = $this->config->get_ldap_link();
      $ldap->cd ($this->dn);
      $release = $this->parent->parent->fai_release;
      $use_dn = preg_replace("/".preg_quote(FAI::get_release_dn($this->dn), '/')."/i", $release, $this->dn);
      new log("remove","fai/".get_class($this),$use_dn,$this->attributes);
      FAI::prepare_to_save_FAI_object($use_dn,array(),true);

      foreach($this->SubObjects as $name => $obj){
        $use_dn = preg_replace("/".preg_quote(FAI::get_release_dn($this->dn), '/')."/i", $release, $obj['dn']);
        FAI::prepare_to_save_FAI_object($use_dn,array(),true);
      }
      $this->handle_post_events("remove");
    }
  }


  /* Save data to object
   */
  function save_object()
  {
    if((isset($_POST['FAIhook_posted'])) && !preg_match("/^freeze/", $this->FAIstate)){
      parent::save_object();
    }
  }

  /* Save to LDAP */
  function save()
  {
    parent::save();

    $ldap = $this->config->get_ldap_link();

    FAI::prepare_to_save_FAI_object($this->dn,$this->attrs);

    if($this->initially_was_account){
      new log("modify","fai/".get_class($this),$this->dn,$this->attributes);
    }else{
      new log("create","fai/".get_class($this),$this->dn,$this->attributes);
    }

    $ldap->cd($this->dn);

    /* Prepare FAIscriptEntry to write it to ldap
     * First sort array.
     *  Because we must delete old entries first.
     * After deletion, we perform add and modify
     */
    $Objects = array();

    /* We do not need to save untouched objects */
    foreach($this->SubObjects as $name => $obj){
      if($obj['status'] != "FreshLoaded"){
        $Objects[$name] = $obj;
      }
    }

    foreach($Objects as $name => $obj){

      foreach($this->sub64coded as $codeIt){
        $obj[$codeIt]=base64_encode(stripslashes($obj[$codeIt]));
      }

      $tmp = array();
      $attributes = array_merge($this->sub_Load_Later,$this->subAttributes);
      foreach($attributes as $attrs){
        if(!isset($obj[$attrs])) continue;
        if($obj[$attrs] == ""){
          $obj[$attrs] = array();
        }
        if(!is_array($obj[$attrs])){
          $tmp[$attrs] = stripslashes($obj[$attrs]);
        }else{
          $tmp[$attrs] = $obj[$attrs];
        }
      }

      $tmp['objectClass'] = $this->subClasses;

      $sub_dn = "cn=".$obj['cn'].",".$this->dn;

      if($obj['status']=="new"){
        $ldap->cat($sub_dn,array("objectClass"));
        if($ldap->count()){
          $obj['status']="edited";
        }
      }

      if($obj['status'] == "delete"){
        FAI::prepare_to_save_FAI_object($sub_dn,array(),true);
        $this->handle_post_events("remove");
      }elseif($obj['status'] == "edited"){
        FAI::prepare_to_save_FAI_object($sub_dn,$tmp);
        $this->handle_post_events("modify");
      }elseif($obj['status']=="new"){
        FAI::prepare_to_save_FAI_object($sub_dn,$tmp);
        $this->handle_post_events("add");
      }
    }
  }


  /* Return plugin informations for acl handling */
  static function plInfo()
  {
    return array(
      'plShortName'   => _('Hook'),
      'plDescription' => _('FAI hook'),
      'plSelfModify'  => FALSE,
      'plPriority'    => 20,
      'plObjectType'  => array('faiHook' => array(
        'name'        => _('FAI hook'),
        'filter'      => 'objectClass=FAIhook',
        'aclCategory' => 'fai'
      )),
      'plProvidedAcls' => array(
        'cn'                => _('Name').' ('._('Read only').')',
        'description'       => _('Description')
      )
    );
  }
}
?>
