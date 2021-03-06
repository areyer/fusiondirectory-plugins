<?php

/*
  This code is part of FusionDirectory (http://www.fusiondirectory.org/)
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

class puppetNode extends simplePlugin
{
  var $displayHeader  = TRUE;
  var $objectclasses  = array("puppetClient");

  // The main function : information about attributes
  static function getAttributesInfo ()
  {
    return array(
      // Attributes are grouped by section
      'section1' => array(
        'name'  => _("Puppet node settings"),
        'attrs' => array(
          new SetAttribute (
            new StringAttribute (_("Puppet class"), _("Puppet Node Class"), "puppetClass")
          ),
          new SelectAttribute (_("Parent node"), _("Puppet Parent Node"),       "parentNode", FALSE),
          new SelectAttribute (_("Environment"), _("Puppet Node Environment"),  "environment", TRUE),
          new SetAttribute (
            new CompositeAttribute (
              _("A variable setting for puppet"),
              'puppetVar',
              array(
                new StringAttribute ('',   _('Name of the variable'),   'puppetVar'),
                new StringAttribute ('=',  _('Value of the variable'),  'puppetVarValue'),
              ),
              '%[^ =]=%[^ ]', // sscanf format
              '%s=%s',        // sprintf format
              '',             // acl, empty string means we don't care (handle it automagically)
              'Puppet Vars'
            )
          ),
        )
      ),
    );
  }

  // We also need this function that return some information about the plugin
  static function plInfo ()
  {
    return array(
      "plShortName"       => _("Puppet"),
      "plDescription"     => _("Support for puppet schema in order to edit puppet classes and puppet vars"),
      "plSelfModify"      => FALSE,
      "plObjectType"      => array("server", "workstation"),

      // But simplePlugin can generate the ACL list for us
      "plProvidedAcls"    => parent::generatePlProvidedAcls(self::getAttributesInfo())
    );
  }

  function __construct(&$config, $dn = NULL, $object = NULL)
  {
    parent::__construct($config, $dn, $object);

    $ldap = $this->config->get_ldap_link();
    $ldap->cd($this->config->current['BASE']);
    $ldap->search("(objectClass=puppetClient)", array('cn'));
    $nodes = array();
    while ($attrs = $ldap->fetch()) {
      $nodes[] = $attrs['cn'][0];
    }
    $this->attributesInfo['section1']['attrs']['parentNode']->setChoices($nodes);
    $ldap->search("(objectClass=puppetServer)", array('puppetAvailableEnvironment'));
    if ($ldap->count() == 0) {
      $this->is_account = FALSE; // TODO : Add something to simplePlugin to handle this case : the tab should not exist
      return;
    }
    $attrs = $ldap->fetch();
    $envs = $attrs['puppetAvailableEnvironment'];
    unset($envs['count']);
    $this->attributesInfo['section1']['attrs']['environment']->setChoices($envs);
    //~ $this->attributesInfo['section1']['attrs']['puppetClass']->attribute->setChoices(array('class1','class2'));
  }
}

?>
