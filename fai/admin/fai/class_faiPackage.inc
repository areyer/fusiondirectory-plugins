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

class faiPackage extends faiPluginClass
{
  /* attribute list for save action */
  var $ignore_account   = TRUE;

  /* Attributes for this Object */
  var $attributes       = array("cn","description","FAIpackage","FAIdebianRelease","FAIdebianSection", "FAIinstallMethod");

  /* ObjectClasses for this Object*/
  var $objectclasses    = array("top","FAIclass","FAIpackageList","FAIrepository");

  /* Class name of the Ldap ObjectClass for the Sub Object */
  var $subClass         = "";
  var $subClasses       = array("top","FAIclass","FAIscriptEntry");

  /* Class name of the php class which allows us to edit a Sub Object */
  var $subClassName     = "";

  /* Attributes to initialise for each subObject */
  var $subAttributes    = array("cn","description");
  var $sub64coded       = array();

  var $ConfiguredPackages = array();

  /* Specific attributes */
  var $cn               = "";       // The class name for this object
  var $description      = "";       // The description for this set of partitions
  var $is_dialog        = false;    // specifies which buttons will be shown to save or abort
  var $SubObjects       = array();  // All leafobjects of this object

  var $FAIdebianRelease = ""; // The selected release
  var $FAIcustomRelease = array(); // The custom releases in use
  var $FAIdebianSection = array(); // selected section
  var $FAIinstallMethod = "aptitude"; // hard coded

  var $sections         = array();  // All section types

  var $list             = NULL;

  var $usedPackages     = array();
  var $buffer           = array();
  var $newDialogShown   = false;

  var $FAIstate         = "";
  var $view_logged      = FALSE;
  var $base;
  var $FAIpackage ;

  var $packageSelect ;

  var $FAIinstallMethods  = array( "install", "ninstall", "remove",
      "dselect-upgrade", "taskinst", "taskrm",
      "hold", "clean", "aptitude", "aptitude-r",
      "pending", "dpkgc" );


  function faiPackage (&$config, $dn= NULL)
  {
    /* Load Attributes */
    parent::__construct($config, $dn);

    /* If "dn==new" we try to create a new entry
     * Else we must read all objects from ldap which belong to this entry.
     * First read SubObjects from ldap ... and then the partition definitions for the SubObjects.
     */
    if($dn != "new"){
      $this->dn =$dn;

      /* Check if there are already some packages in this list
       */
      $this->usedPackages = array();
      if(isset($this->attrs['FAIpackage'])){
        unset($this->attrs['FAIpackage']['count']);
        foreach($this->attrs['FAIpackage'] as $pkg){
          $name = preg_replace("/\-$/","",$pkg);
          $this->usedPackages[$name] = $pkg;
        }
        ksort($this->usedPackages);
      }

      /* Fetch package configurations from LDAP */
      $this->ConfiguredPackages = $this->get_configured_packages($this->usedPackages, $this->dn);

      $this->FAIdebianSection = array();
      if(isset($this->attrs['FAIdebianSection'])){
        for($i = 0 ; $i < $this->attrs['FAIdebianSection']['count'] ; $i++ ){
          $sec = $this->attrs['FAIdebianSection'][$i];
          $this->FAIdebianSection[$sec]=$sec;
        }
      }
      $this->FAIpackage = array();

    } // ENDE  dn != new

    $methods = array();
    foreach($this->FAIinstallMethods as $method){
      $methods[$method] = $method;
    }
    $this->FAIinstallMethods = $methods;

    /* Check if we exist already - no need to ask for revisions, etc. then */
    if ($this->dn != "new"){
      $this->newDialogShown= true;
    }
    $this->is_new = FALSE;
    if($this->dn == "new"){
      $this->is_new =TRUE;
    }

    /* Generate package list */
    $this->list= $this->genPkgs();
  }

  function get_configured_packages($packages, $base)
  {
    /* Fetch all package configurations from ldap
     */
    $PackageFilter = "";
    foreach($packages as $name => $value){
      $PackageFilter .= "(FAIpackage=".$name.")";
    }
    /* Also fetch d-i */
    $PackageFilter .= "(FAIpackage=d-i)";

    $PackageFilter = "(&(objectClass=FAIdebconfInfo)(|".$PackageFilter."))";

    /* Search for configuration objects */
    $ldap = $this->config->get_ldap_link();
    $ldap->cd($base);
    $ldap->search($PackageFilter,array("FAIvariable","FAIvariableType",
          "FAIvariableContent","FAIpackage","FAIdebianSection","FAIstate"));

    $configured_packages = array();

    /* Walk through configurations and append them to our list of ConfiguredPackages
     */
    while($attr = $ldap->fetch()){

      /* Skip objects, that are tagged as removed */
      if(isset($object['FAIstate'][0])){
        if(preg_match("/removed$/",$attr['FAIstate'][0])){
          continue;
        }
      }

      $tmp =array();
      $tmp['Name']  = $attr['FAIvariable'][0];
      $tmp['Type']  = $attr['FAIvariableType'][0];
      $tmp['Save']  = TRUE;

      if (isset($attr['FAIvariableContent'][0])){
        if(!in_array($attr['FAIvariableType'],array("multiselect"))){
          $tmp['Value'] = $attr['FAIvariableContent'][0];
        }else{
          $content = array();
          unset($attr['FAIvariableContent']['count']);
          foreach($attr['FAIvariableContent'] as $attr){
            $tmp['Value'][] = $attr;
          }
        }
        $configured_packages[$attr['FAIpackage'][0]][$attr['FAIvariable'][0]]=$tmp;
      }
      else {
        $tmp['Value'] = "";
        $configured_packages[$attr['FAIpackage'][0]][$attr['FAIvariable'][0]]=$tmp;
      }
    }

    return $configured_packages;
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

    /******
     * Initialize a new Package List with release and section name
     ******/

    if (!$this->is_account) {

      /* Assemble release name */
      $release = $this->parent->parent->fai_release;
      $tmp= preg_replace('/[,]*'.preg_quote(get_ou('faiBaseRDN'), '/').'.*$/i', '', $release);
      $tmp= preg_replace('/ou=/', '', $tmp);
      $rev= array_reverse(explode(',', $tmp));
      $this->FAIdebianRelease= "/";
      foreach ($rev as $part){
        $this->FAIdebianRelease.= "/$part";
      }
      $this->FAIdebianRelease= preg_replace('#^[/]*#', '', $this->FAIdebianRelease);

      /* Assemble sections */
      $repos= $this->getServerInfos();
      if(isset($repos[$this->FAIdebianRelease])) {
        $this->FAIdebianSection= $repos[$this->FAIdebianRelease];
        $this->FAIdebianSection= array_unique($this->FAIdebianSection);
      }

      /* Assign Repository settings*/
      $this->is_account     = true;
    }


    /******
     * Add
     ******/

    if (isset($_POST['AddManualpkg']) &&
        isset($_POST['addPpkgsText']) &&
        strlen($_POST['addPpkgsText']) &&
        $this->acl_is_writeable("FAIpackage") &&
        !preg_match("/freeze/",$this->FAIstate)) {

      // Check all splitted packages for valid package names
      $add_packages = preg_split("/[\s,]+/", get_post('addPpkgsText'), -1, PREG_SPLIT_NO_EMPTY);
      if (is_array($add_packages)) {
        $invalid_packages = array();
        foreach ($add_packages as $value) {
          if (!preg_match( "/^[a-z0-9][-0-9a-z+\.]+$/",$value)) {
            $invalid_packages[] = trim($value);
          } else {
            $valid_packages[] = trim($value);
          }
        }
        if (count($invalid_packages)) {
          $str = implode(", ",$invalid_packages);
          msg_dialog::display(_("Invalid package names"),
              sprintf(_("The following package names don't match the Debian policy: %s"),$str),
              ERROR_DIALOG);
        }

        // If we have a complete list of valid packages, add them
        if (count($valid_packages)) {
          foreach ($valid_packages as $value) {
            $this->usedPackages[$value] = $value;
          }

          ksort($this->usedPackages);

          /* Generate package list */
          $this->list = $this->genPkgs();
        }
      }
    }

    // Open the packageSelect dialog to allow adding packages out of a list.
    if(isset($_POST['Addpkg']) && $this->acl_is_writeable("FAIpackage") && !preg_match("/freeze/",$this->FAIstate)){
      session::set('packageSelect_Release',$this->FAIdebianRelease);
      $this->packageSelect = new packageSelect($this->config, get_userinfo());
      $this->dialog = TRUE;
    }

    // Close packageSelect dialog.
    if(isset($_POST['packageSelect_cancel'])){
      $this->packageSelect = FALSE;
      $this->dialog = FALSE;
    }

    // Close packageSelect dialog and add the selected packages.
    if(isset($_POST['packageSelect_save']) && $this->packageSelect instanceOf packageSelect){
      if (!preg_match("/freeze/i", $this->FAIstate)) {
        $use = $this->packageSelect->save();

        foreach($use as $pkg) {
          $item = array();
          for ($i =0; $i < $pkg['count']; $i++) {
            $item[$pkg[$i]] = $pkg[$pkg[$i]][0];
          }
          $this->usedPackages[$item['PACKAGE']] = $item['PACKAGE'];
        }

        ksort($this->usedPackages);

        /* Generate package list */
        // We could do a bit more optimised here as the $use array much likely contains the release or customs associated to each package.
        $this->list = $this->genPkgs();
      }
      $this->packageSelect = FALSE;
      $this->dialog = FALSE;
    }

    // Display package add dialog
    if($this->packageSelect instanceOf packageSelect){

      session::set('filterBlacklist', array('PACKAGE' => $this->usedPackages));
      return $this->packageSelect->execute();
    }


    /* Check image Posts
     */
    foreach($_POST as $name => $value){

      /******
       * Mark as removed
       ******/

      if(!preg_match('/freeze/i', $this->FAIstate) && preg_match("/^removal_package_/",$name)){
        $id = @base64_decode(preg_replace("/^removal_package_(.*)_[xy]*$/","\\1",$name));
        if(isset($this->usedPackages[$id])){
          $pkg = $this->usedPackages[$id];
          if (preg_match('/\-$/', $pkg)){
            $pkg= preg_replace('/\-$/', '', $pkg);
          } else {
            $pkg= preg_replace('/$/', '-', $pkg);
          }
          $this->usedPackages[$id] = $pkg;
        }
        break;
      }

      /******
       * Delete Pkgs
       ******/
      if(!preg_match('/freeze/i', $this->FAIstate) && preg_match("/^remove_package_/",$name)){
        $id = @base64_decode(preg_replace("/^remove_package_(.*)_[xy]*$/","\\1",$name));
        if(isset($this->usedPackages[$id])){
          unset($this->usedPackages[$id]);
        }
        break;
      }

      /******
       * Configure Pkgs
       ******/
      if(!preg_match('/freeze/i', $this->FAIstate) && preg_match("/^configure_package_/",$name)){
        $pkg = @base64_decode(preg_replace("/^configure_package_(.*)_[xy]*$/","\\1",$name));

        if(isset($this->usedPackages[$pkg])){

          /* Configuration dialog open*/
          $pkg_config = array();
          if(isset($this->ConfiguredPackages[$pkg])){
            $pkg_config = $this->ConfiguredPackages[$pkg];
          }
          $this->dialog = new faiPackageConfiguration($this->config, $this->dn,$pkg, $this->FAIdebianRelease , $pkg_config);
          $this->is_dialog =true;
        }
        break;
      }
    }

    /* Abort package selection dialog */
    if(isset($_POST['CancelSubObject'])){
      $this->dialog = false;
      $this->is_dialog=false;
    }

    /* Save Configuration */
    if(isset($_POST['SaveObjectConfig'])){
      if(!preg_match("/^freeze/", $this->FAIstate)){
        $this->ConfiguredPackages= array_merge($this->ConfiguredPackages,$this->dialog->save());
      }
      $this->dialog = false;
      $this->is_dialog=false;
    }

    /* cancel configuration */
    if(isset($_POST['CancelObjectConfig'])){
      $this->dialog = false;
      $this->is_dialog=false;
    }

    /* Display dialog */
    if($this->is_dialog){
      $this->dialog->save_object();
      return $this->dialog->execute();
    }


    /******
     * Display UI / HTML / smarty
     ******/

    /* Create divselectbox to display a list of all currently used packages */
    $divselectbox = new divSelectBox("faiPackages");

    ksort($this->usedPackages);
    if(is_array($this->usedPackages)){
      foreach($this->usedPackages as $usedName => $name){

        $actions = "";

        /* Append message if package is configured */
        $configured = "<img src='images/empty.png' alt=' '  class='center'>";
        if(isset($this->ConfiguredPackages[$usedName])){
          $configured = "<img src='plugins/fai/images/package_configure.png' alt='"._("Configured")."'
                      title='"._("Configured")."' class='center'>";
        }

        /* Adapt used name if we're marked for removal */
        $removal = "<img src='images/empty.png' alt=' '  class='center'>";
        if (preg_match('/\-$/', $name)){
          $removal = "<img src='plugins/fai/images/removal_mark.png' alt='"._("Package marked for removal")."'
                      title='"._("Package marked for removal")."' class='center'>";
        }

        /* Get Version */
        $version = "&nbsp;";
        if(isset($this->list[$usedName]['VERSION'])){
          $version = $this->list[$usedName]['VERSION'];
        }

        /* Get description */
        $description = "&nbsp;";
        if(isset($this->list[$usedName]['DESCRIPTION'])){
          $description = base64_decode($this->list[$usedName]['DESCRIPTION']);
        }

        if(!preg_match('/^freeze/', $this->FAIstate) && $this->acl_is_writeable("FAIpackage")){
          $actions = "<input type='image' class='center' title='"._("Mark package for removal")."'
            src='plugins/fai/images/removal_mark.png' name='removal_package_".base64_encode($usedName)."' >";
        }

        if(isset($this->list[$usedName]['HASTEMPLATE']) &&
           !preg_match('/^freeze/', $this->FAIstate) && $this->acl_is_writeable("FAIdebconfInfo")){
          $actions.= "&nbsp;<input type='image' class='center' title='"._("Configure this package")."'
            src='plugins/fai/images/package_configure.png' name='configure_package_".base64_encode($usedName)."' >";
        }
        if(!preg_match('/^freeze/', $this->FAIstate) && $this->acl_is_writeable("FAIpackage")){
          $actions.= "&nbsp;<input type='image' class='center' title='"._("Remove this package")."'
            src='geticon.php?context=actions&icon=edit-delete&size=16' name='remove_package_".base64_encode($usedName)."' >";
        }

        $field1 = array("html" => $configured."&nbsp;".$removal,"attach" => "style='width:40px;'");
        $field2 = array("html" => $usedName, "attach" => "style='width:200px;'");
        $field3 = array("html" => $version);
        $field4 = array("html" => $description);
        $field5 = array("html" => $actions, "attach" => "style='width:60px; border-right:0px;'");
        $divselectbox->AddEntry(array($field1,$field2,$field3,$field4,$field5));
      }
    }

    /* Assign variables */
    foreach($this->attributes as $attrs){
      $smarty->assign($attrs,$this->$attrs);
    }
    $smarty->assign( "FAIinstallMethods", $this->FAIinstallMethods );

    /* Assign section to smarty */
    $strsec = "";
    foreach($this->FAIdebianSection as $sec){
      $strsec .= $sec." ";
    }

    $tmp = $this->plInfo();
    foreach($tmp['plProvidedAcls'] as $name => $translated){
      $smarty->assign($name."ACL",$this->getacl($name,preg_match("/freeze/",$this->FAIstate)));
    }

    $smarty->assign("freeze",       preg_match("/freeze/", $this->FAIstate));
    $smarty->assign("divselectbox", $divselectbox->DrawList());
    $smarty->assign("release",      $this->FAIdebianRelease);
    $smarty->assign("sections",     $this->sections);
    $smarty->assign("section",      $strsec);
    $display.= $smarty->fetch(get_template_path('faiPackage.tpl', TRUE));
    return($display);
  }


  /*! \brief  Removes this packageList from the ldap database
   */
  function remove_from_parent()
  {
    $ldap = $this->config->get_ldap_link();
    $ldap->cd ($this->dn);
    $release = $this->parent->parent->fai_release;
    $use_dn = preg_replace("/".preg_quote(FAI::get_release_dn($this->dn), '/')."/i", $release, $this->dn);
    FAI::prepare_to_save_FAI_object($use_dn,array(),true);
    new log("remove","fai/".get_class($this),$use_dn,$this->attributes);
    foreach($this->ConfiguredPackages as $pkgname => $attrs){
      foreach($attrs as $name => $attr){
        $pkgdn =  "FAIvariable=".$name.",".$this->dn;
        $use_dn = preg_replace("/".preg_quote(FAI::get_release_dn($this->dn), '/')."/i", $release, $pkgdn);
        FAI::prepare_to_save_FAI_object($use_dn,array(),true);
      }
    }
    $this->handle_post_events("remove");
  }


  /*! \brief  Collect all relevant POST vars for this plugin
   */
  function save_object()
  {
    if(preg_match("/^freeze/", $this->FAIstate)) return;
    parent::save_object();
  }


  /*! \brief  Check given inputs for this package list
      @return Array Containing all error messages, or an empty array if no error occured
   */
  function check()
  {
    /* Call common method to give check the hook */
    $message = parent::check();

    if (count($this->usedPackages) == 0) {
      $message[]= _("Please select a least one package!");
    }

    if ((empty($this->FAIdebianRelease)) || (empty($this->FAIdebianSection))) {
      $message[] = _("Please choose a valid release/section combination for your repository setup!");
    }

    return $message;
  }


  /*! \brief  Reload the list of cached packages.
      @return Returns the currently cached list of packages.
   */
  function genPkgs()
  {
    if (empty($this->FAIdebianRelease)) {
      return;
    }

    if (empty($this->usedPackages)) {
      return; // If there are no packages, no need to do anything
    }

    $q = new supportDaemon();
    $attrs = array("distribution", "package","version", "section", "description", "timestamp","hastemplate");

    $packages = array_diff_key($this->usedPackages,$this->buffer); // packages names that are not already in the buffer

    $release = $this->FAIdebianRelease;
    $customs = $q->FAI_get_customs($release);
    for ($i = -1; (count($packages) > 0) && ($i < count($customs)); $i++) {
      if ($i>=0) { // if release has been handled
        $release = $customs[$i]; // take the next custom
      }
      $ret = $q->FAI_get_packages($release,$attrs,array_keys($packages),-1,-1,true);
      if ($q->is_error()) {
        msg_dialog::display(_("Service infrastructure"),msgPool::siError($q->get_error()),ERROR_DIALOG);
        return $this->buffer;
      } else {
        foreach ($ret as $attr) {
          $this->buffer[$attr['PACKAGE']] = $attr;
          echo "found ".$attr['PACKAGE']."<br\>\n";
          unset($packages[$attr['PACKAGE']]); // remove found packages
          if ($i>=0) {
            $this->FAIcustomRelease[$release] = $release;
          }
        }
      }
      @DEBUG (DEBUG_TRACE, __LINE__, __FUNCTION__, __FILE__,count($packages),"$release done, packages left");
    }

    if (count($packages) > 0) {
      msg_dialog::display(_("Unkown packages"),"The following packages were not found on the repository : ".join(",",$packages),ERROR_DIALOG);
      foreach($packages as $p=>$pp) {
        unset($this->usedPackages[$p]);
      }
    }

    return $this->buffer;
  }


  /*! \brief Save packages and their configuration to ldap
   */
  function save()
  {

    /* Assemble release name */
    if ($this->FAIdebianRelease == "ClearFromCopyPaste") {

      $current_release = $this->parent->parent->fai_release;
      $tmp = preg_replace('/,'.preg_quote(get_ou('faiBaseRDN'), '/').'.*$/i', '', $current_release);
      $tmp = preg_replace('/ou=/', '', $tmp);
      $rev = array_reverse(explode(',', $tmp));
      $this->FAIdebianRelease= "";
      foreach ($rev as $part){
        $this->FAIdebianRelease.= "/$part";
      }
      $this->FAIdebianRelease= preg_replace('#^/#', '', $this->FAIdebianRelease);
    }

    parent::save();

    $ldap = $this->config->get_ldap_link();

    $this->attrs['FAIpackage'] = array();
    foreach($this->usedPackages as $pkg){
      $this->attrs['FAIpackage'][] = $pkg;
    }

    $this->attrs['FAIdebianSection'] = array();
    foreach($this->FAIdebianSection as $sec){
      $this->attrs['FAIdebianSection'][] = $sec;
    }

    if (!empty($this->FAIcustomRelease)) {
      $this->attrs['FAIcustomRelease'] = array();
      foreach ($this->FAIcustomRelease as $rel) {
        $this->attrs['FAIcustomRelease'][] = $rel;
      }
    }

    FAI::prepare_to_save_FAI_object($this->dn,$this->attrs);

    if ($this->initially_was_account) {
      new log("modify","fai/".get_class($this),$this->dn,$this->attributes);
    } else {
      new log("create","fai/".get_class($this),$this->dn,$this->attributes);
    }

    $ldap->cd($this->dn);

    /* Save Package configurations */
    foreach($this->ConfiguredPackages as $pkgname => $attrs){
      foreach($attrs as $name => $attr){

        $pkgattrs = array();

        foreach($attr as $n=>$v){
          if(empty($v)) $v = array();
        }

        /* Set attributes */
        $pkgattrs['objectClass'][]        = "FAIdebconfInfo";

        $pkgattrs['FAIpackage']           = $pkgname;
        $pkgattrs['FAIvariable']          = $name;
        $pkgattrs['FAIvariableType']      = $attr['Type'];
        $pkgattrs['FAIvariableContent']   = $attr['Value'];
        $pkgdn =  "FAIvariable=".$name.",".$this->dn;

        if (in_array($pkgattrs['FAIvariableType'],array("boolean","multiselect","password","select","string","text"))) {
          if ($pkgattrs['FAIvariableType'] == "text" && $pkgattrs['FAIvariableContent'] == "") {
            fusiondirectory_log("Skipped saving FAIvariable '$name' empty string can't be saved.");
          } else {
            FAI::prepare_to_save_FAI_object($pkgdn,$pkgattrs);
          }
        }
      }
    }
  }


  /*! \brief  Return plugin informations for acl handling
      @return Array ACL infos of this plugin.
   */
  static function plInfo()
  {
    return array(
      'plShortName'   => _('Package'),
      'plDescription' => _('FAI Package list'),
      'plSelfModify'  => FALSE,
      'plPriority'    => 28,
      'plObjectType'  => array('faiPackage' => array(
        'name'        => _('FAI Package'),
        'filter'      => 'objectClass=FAIpackageList',
        'aclCategory' => 'fai'
      )),

      'plProvidedAcls' => array(
        'cn'                => _('Name'),
        'description'       => _('Description'),
        'FAIpackage'        => _('Packages'),
        'FAIdebianSection'  => _('Section').'&nbsp;('._('Readonly').')',
        'FAIinstallMethod'  => _('Install Method'),
        'FAIdebconfInfo'    => _('Package configuration'),
        'FAIdebianRelease'  => _('Release').'&nbsp;('._('Readonly').')'
      )
    );
  }


  /*! \brief  Returns a list of all configured servers with repositories.
      @return Array  All repository server
   */
  function getServerInfos()
  {
    $ret = array();
    $o_queue = new supportDaemon();
    $servs = $o_queue->FAI_get_server();
    foreach ($servs as $serv) {
        $ret[$serv["FAI_RELEASE"]] = $serv["SECTIONS"];
    }
    return $ret;
  }
}
?>
