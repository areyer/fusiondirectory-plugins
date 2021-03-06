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

class faiPartitionTableEntry extends plugin
{
  /* attribute list for save action */
  var $ignore_account= TRUE;
  var $attributes   = array("DISKcn","DISKdescription");
  var $objectclasses= array();

  var $DISKcn              = "";
  var $DISKdescription     = "";

  var $partitions           = array();
  var $UsedAttrs            = array();
  var $is_edit              = false;
  var $old_cn               = "";
  var $parent               = null;

  function faiPartitionTableEntry (&$config, $dn= NULL,&$parent,$disk=false)
  {
    parent::__construct($config, $dn);
    $this->UsedAttrs  = array("cn","description","FAIpartitionType","FAIpartitionNr","FAIfsType","FAImountPoint","FAIpartitionSize",
                         "FAImountOptions","FAIfsOptions","FAIpartitionFlags");

    $this->parent = $parent;

    /* We want to edit an entry */
    if($disk){

      /* Walk through partitions */
      foreach($disk['partitions'] as $name => $values){

        $disk['partitions'][$name]['old_cn']= $disk['partitions'][$name]['cn'];

        /* Assign empty attributes, if attribute is missing */
        foreach($this->UsedAttrs as $attr){
          if(!isset($values[$attr])){
            $disk['partitions'][$name][$attr]="";
          }
        }

        if (preg_match('/^_/', $disk['partitions'][$name]['FAIfsType'])){
          $disk['partitions'][$name]['FAIfsType']= preg_replace('/^_/', '', $disk['partitions'][$name]['FAIfsType']);
        }
      }

      /* Set default attributes */
      $this->DISKcn          = $disk['cn'];
      $this->DISKdescription = $disk['description'];
      $this->partitions       = $disk['partitions'];
      $this->is_edit          = true;
      $this->old_cn           = $disk['cn'];
    }
  }


  function execute()
  {
    /* Call parent execute */
    parent::execute();

    /* Fill templating stuff */
    $smarty     = get_smarty();
    $s_action   = "";
    $s_entry    = "";
    $display    = "";

    /* Assign values
     * And Checkbox selection
     */
    foreach($this->attributes as $attrs){
      $smarty->assign($attrs,$this->$attrs);
      if($this->$attrs){
        $smarty->assign($attrs."CHK"," ");
      }else{
        $smarty->assign($attrs."CHK"," disabled ");
      }
    }

    /* Check all Posts if there is something usefull for us,
     * For example : Delete is posted as Delete_1
     * The number specifies the index we want to delete
     */
    foreach($_POST as $name => $value){
      if((preg_match("/Delete_.*/",$name)) && $this->acl_is_removeable() && !preg_match("/freeze/i",$this->FAIstate)){
        $tmp = explode("_",$name);
        $s_action = "remove";
        $s_entry  = $tmp[1];
      }
    }

    /* To remove a partition we unset the index posted.
     * We must sort the index again, else we possibly got problems
     * with partitions order.
     */
    if($s_action == "remove" && $this->acl_is_removeable() && !preg_match("/freeze/i",$this->FAIstate)){
      unset($this->partitions[$s_entry]);
      $tmp= array();
      foreach($this->partitions as $part){
        $tmp[count($tmp)+1]=$part;
      }
      $this->partitions = $tmp;
    }

    /* To add a partitions we only append an empty
     *  array to the already defined partitions.
     */
    if(isset($_POST['AddPartition']) && $this->acl_is_createable() && !preg_match("/freeze/i",$this->FAIstate)){
      foreach($this->UsedAttrs as $attr){
        $tmp[$attr]                = "";
      }
      $tmp["old_cn"]               = "";
      $this->partitions[count($this->partitions)+1]=$tmp;
    }

    /* $setup contains a table with the partitions.
     */
    $smarty->assign("setup", $this->generateParts());
    foreach($this->attributes as $attr){
      $smarty->assign($attr,$this->$attr);
    }

    $tmp = $this->plInfo();
    $sacl = "";
    foreach($tmp['plProvidedAcls'] as $name => $translated){
      $acl = $this->getacl($name, preg_match("/freeze/i",$this->FAIstate));
      $smarty->assign($name."ACL",$acl);
    }

    $smarty->assign("sub_object_is_createable",$this->acl_is_createable());
    $smarty->assign("freeze",preg_match("/freeze/i",$this->FAIstate));

    /* Fetch template and show the result
     */
    $display.= $smarty->fetch(get_template_path('faiPartitionTableEntry.tpl', TRUE));
    return($display);
  }

  function generateParts()
  {
    /* Define Arrays with allowed syntax */
    $PartitionTypes   = array("primary"=>_("primary"),"logical"=>_("logical"));
    $FAIfsTypes       = array("ext2","ext3","xfs","swap","reiser","dosfat16","winfat32");

    /* Display Header */
    $str = "";
    if (count($this->partitions)){
      $str .= "<tr>";
      $str .= "<td><b>"._("Type")."</b></td>";
      $str .= "<td><b>"._("FS type")."</b></td>";
      $str .= "<td><b>"._("Mount point")."</b></td>";
      $str .= "<td><b>"._("Size in MB")."</b></td>";
      $str .= "<td><b>"._("Mount options")."</b></td>";
      $str .= "<td><b>"._("FS option")."</b></td>";
      $str .= "<td><b>"._("Preserve")."</b></td>";
      $str .= "<td>&nbsp;</td>";
      $str .= "</tr>";
    }

    /* Walk through all defined partitions.
     * Create a new row for each partition and append it to
     *  the header defined above.
     * To be able to check the posts later, we append a key to each single postfield. like cn_1 ... cn_2
     */
    foreach($this->partitions as $key => $part){

      $dis = "";
      if($part['FAIpartitionFlags'] == "preserve"){
        $dis = " disabled ";
      }

      $disableALL = "";
      if (preg_match("/freeze/", $this->FAIstate)){
        $disableALL = " disabled ";
      }

      /* Generate Partition select box
       */
      $PartitionType = "";
      if($this->acl_is_writeable("FAIpartitionType")){
        $PartitionType = "<select name='FAIpartitionType_".$key."' id='FAIpartitionType_".$key."' ".$disableALL.">";
        foreach($PartitionTypes as $type => $PTname){
          if($part['FAIpartitionType'] == $type){
            $PartitionType .="<option value='".$type."' selected >".$PTname."</option>";
          }else{
            $PartitionType .="<option value='".$type."'>".$PTname."</option>";
          }
        }
        $PartitionType.="</select>";
      }elseif($this->acl_is_readable("FAIpartitionType")){
        $PartitionType = $part['FAIpartitionType'];
      }


      /* Generate fsType select box
       */
      $FAIfsType= "";
      if($this->acl_is_writeable("FAIfsType")){
        $FAIfsType= "<select name='FAIfsType_".$key."' id='FAIfsType_".$key."' ".$dis." ".$disableALL.">";
        foreach($FAIfsTypes as $type){
          if($part['FAIfsType'] == $type){
            $FAIfsType  .="<option value='".$type."' selected >".$type."</option>";
          }else{
            $FAIfsType .="<option value='".$type."'>".$type."</option>";
          }
        }
        $FAIfsType.="</select>";
      }elseif($this->acl_is_readable("FAIfsType")){
        $FAIfsType = $part['FAIfsType'];
      }

      foreach(array("FAImountPoint","FAIpartitionSize","FAImountOptions","FAIfsOptions") as $attr){
        $$attr  = "";
        if($this->acl_is_writeable($attr)){
          $$attr = "<input name='".$attr."_".$key."'  ".$disableALL."  ".$dis."
            value='".$part[$attr]."'    id='".$attr."_".$key."'>";

        }elseif($this->acl_is_readable($attr)){
          $$attr = $part[$attr];
        }
      }


      $str .= "\n<tr>";
      $str .= "\n<td>".$PartitionType."</td>";
      $str .= "\n<td>".$FAIfsType."</td>";
      $str .= "\n<td>".$FAImountPoint."</td>";
      $str .= "\n<td>".$FAIpartitionSize."</td>";
      $str .= "\n<td>".$FAImountOptions."</td>";
      $str .= "\n<td>".$FAIfsOptions."</td>";

      $changeState = "onClick=\"changeState('FAImountPoint_".$key."') ; ".
        "changeState('FAIpartitionSize_".$key."') ; ".
        "changeState('FAImountOptions_".$key."') ; ".
        "changeState('FAIfsType_".$key."') ; ".
        "changeState('FAIfsOptions_".$key."') ; \"";

      if($this->acl_is_writeable("FAIpartitionFlags")){
        if($part['FAIpartitionFlags']=="preserve"){
          $str .= "\n<td><input type='checkbox' name='FAIpartitionFlags_".$key."' value='preserve' checked ".$changeState." ".$disableALL."></td>";
        }else{
          $str .= "\n<td><input type='checkbox' name='FAIpartitionFlags_".$key."' value='preserve' ".$changeState." ".$disableALL."></td>";
        }
      }else{
        if($part['FAIpartitionFlags']=="preserve"){
          $str .= "<td>"._("False")."</td>";
        }else{
          $str .= "<td>"._("True")."</td>";
        }
      }
      if($this->acl_is_removeable()){
        $str .= "\n<td><input type='submit' name='Delete_".$key."' value='"._("Remove")."' ".$disableALL."></td>";
      }else{
        $str .= "\n<td><input type='submit' name='restricted' disabled value='"._("Remove")."'></td>";
      }
      $str .= "\n</tr>";
    }

    if(!empty($str)){
      $str = "<table style='width:100%'>".$str."</table>";
    }
    return($str);

  }

  function save()
  {
    $tmp = array();
    $tmp['cn']          = $this->DISKcn;

    /* Attach partitions */
    foreach($this->partitions as $key=>$val) {
      $this->partitions[$key]['FAIpartitionNr']=$key;
      $this->partitions[$key]['cn']=$key;
    }

    $tmp['description'] = $this->DISKdescription;
    $tmp['partitions']  = $this->partitions;

    /* If hdd name has changed, tell partitionTable to rename it */
    if(($this->is_edit)&&($this->old_cn != $this->DISKcn)){
      $tmp['rename']['from']  = $this->old_cn;
      $tmp['rename']['to']    = $this->DISKcn;
    }
    $tmp['FAIdiskType'] = "old";
    return($tmp);
  }


  /* Save data to object */
  function save_object()
  {
    if((isset($_POST['TableEntryFrameSubmitted'])) && !preg_match("/freeze/", $this->FAIstate) ){
      parent::save_object();

      foreach($this->partitions as $key => $part){
        foreach($this->UsedAttrs as $attrs){

          if($this->acl_is_writeable($attrs)){

            if(isset($_POST[$attrs."_".$key])){
              $this->partitions[$key][$attrs] = $_POST[$attrs."_".$key];
            }else{
              $this->partitions[$key][$attrs] = false;
            }
          }

          if(!$this->partitions[$key]['FAIpartitionFlags']) {
            $this->partitions[$key]['FAIpartitionFlags'] = " ";
          }
        }

        /* Adapt ext3 to have -j option */
        if ($this->partitions[$key]["FAIfsType"] == "ext3") {
          $this->partitions[$key]["FAIfsOptions"]= preg_replace('/\s*-j\s*/', '', $this->partitions[$key]["FAIfsOptions"]);
          $this->partitions[$key]["FAIfsOptions"]= "-j ".$this->partitions[$key]["FAIfsOptions"];
        }
      }

    }
  }


  /* Check supplied data */
  function check()
  {
    /* Call common method to give check the hook */
    $message= plugin::check();

    /* Check for an empty disk name */
    $d = trim($this->DISKcn);
    if($d == "" ){
      $message[] = msgPool::required(_("Name"));
    }
    if(preg_match("/[^a-z0-9_\-]/i",$d)){
      $message[] = msgPool::invalid(_("Name"),$d,"/[a-z0-9_\-]/i");
    }

    if($this->old_cn != $this->DISKcn && in_array($this->DISKcn,$this->parent->getUsedDiskNames())){
      $message[] = msgPool::duplicated(_("Name"));
    }

    /* check every partition.
     * if there is an invalid value defined, append an errorstr to message
     */

    /* Array that contain every partitionname mountpoint etc already assigned */
    $alreadyUsed    = array();
    foreach($this->UsedAttrs as $attrs){
      $alreadyUsed[$attrs] = array();
    }

    foreach($this->partitions as $key => $part){

      /* Skip all checks, if preserve is set */
      if($part['FAIpartitionFlags'] == "preserve"){
        $this->partitions[$key]['FAImountPoint']    = "-";
        $this->partitions[$key]['FAIfsType']        = "preserve";
        $this->partitions[$key]['FAIpartitionSize'] = "preserve";
        continue;
      }

      if((in_array($part['FAImountPoint'],$alreadyUsed['FAImountPoint']))&&($part['FAIfsType']!="swap")){
        $message[]=sprintf(_("please enter a unique mount point for partition %s"),($key));
      }

      if($part['FAIfsType']!="swap" && $part['FAImountPoint'] != "-"){
        if((empty($part['FAImountPoint']))||(!((preg_match("/^\/.*/",$part['FAImountPoint']))||(preg_match("/^swap$/",$part['FAImountPoint']))))){
          $message[]= msgPool::invalid(sprintf(_("partition %s mount point"),$key));
        }
      }
      if($part['FAIfsType'] == "swap"){
        if(in_array($part['FAIfsType'],$alreadyUsed['FAIfsType'])){
          $message[]=sprintf(_("File system type 'swap' is already used, change file system type for partition %s."),$key);
        }
      }
      if(($part['FAIfsType'] == "swap")&&(!empty($part['FAImountPoint']))&&($part['FAImountPoint']!="swap")){
        $message[]=_("Please use 'swap' as mount point, if 'swap' is used as fs-type.");
      }

      $tmp = explode("-",$part['FAIpartitionSize']);
      switch (count($tmp)){
        case 0:
                $message[]= msgPool::invalid(sprintf(_("partition %s size"),$key));
                break;
        case 1:
                if (!tests::is_id($tmp[0]) &&(!empty($tmp[1]))){
                  $message[]=  msgPool::invalid(sprintf(_("partition %s size"),$key));
                }
                break;

        case 2:
                if( !tests::is_id($tmp[0]) && !tests::is_id($tmp[1]) && !empty($tmp[1]) ){
                  $message[]= msgPool::invalid(sprintf(_("partition %s size"),$key));
                }elseif(!empty($tmp[1]) && $tmp[0]>=$tmp[1]){
                  $message[]= msgPool::invalid(sprintf(_("partition %s size"),$key));
                }
                break;

        default:
                $message[]= msgPool::invalid(sprintf(_("partition %s size"),$key));
      }

      foreach($this->UsedAttrs as $attrs){
        $alreadyUsed[$attrs][$key] = $part[$attrs];
      }
    }

    $cnt = 0;
    foreach($this->partitions as $key => $part){
      if($part['FAIpartitionType'] == "primary"){
        $cnt ++ ;
      }
    }
    if($cnt > 3){
      $message[] = _("You have more than 3 primary partition table entries in your configuration, please check your configuration twice.");
    }

    return ($message);
  }



  /* Return plugin informations for acl handling */
  static function plInfo()
  {
    return array(
      "plShortName"   => _("Partition table entry"),
      "plDescription" => _("FAI partition table entry"),
      "plSelfModify"  => FALSE,
      "plPriority"    => 27,
      "plCategory"    => array("fai"),

      "plProvidedAcls" => array(
        "DISKcn"            => _("Name"),
        "DISKdescription"   => _("Description"),
        "FAIpartitionType"  => _("Partition type"),
        "FAIpartitionNr"    => _("Partition no."),
        "FAIfsType"         => _("File system type"),
        "FAImountPoint"     => _("Mount point"),
        "FAIpartitionSize"  => _("Partition size"),
        "FAImountOptions"   => _("Mount options"),
        "FAIfsOptions"      => _("File system options"),
        "FAIpartitionFlags" => _("Partition flags")
      )
    );
  }
}
?>
