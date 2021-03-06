<?php
/*
  This code is part of FusionDirectory (http://www.fusiondirectory.org/)
  Copyright (C) 2013  FusionDirectory

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

class autofsManagement extends simpleManagement
{
  protected $objectTypes  = array('nisMap', 'nisObject');

  protected $autoFilterAttributes = array('dn', 'cn', 'description', 'nisMapName');

  /* Return plugin informations for acl handling */
  static function plInfo()
  {
    return array(
      'plShortName'   => _('Auto fs'),
      'plDescription' => _('Auto fs management'),
      'plIcon'        => 'plugins/autofs/images/icon.png',
      'plPriority'    => 24,
      'plSection'     => 'admin',
      'plManages'     => array('nisMap', 'nisObject'),

      'plProvidedAcls' => array()
    );
  }

  function parseXML ($file)
  {
    $data = parent::parseXML($file);
    $data['list']['table']['layout'] = '|20px;c||||150px;r|';
    $column = array(
      'label'         => 'Mount point',
      'sortAttribute' => 'nisMapName',
      'sortType'      => 'string',
      'value'         => '%{filter:link(row,dn,"%s",nisMapName)}',
      'export'        => 'true',
    );
    array_splice($data['list']['table']['column'], 2, 0, array($column));
    return $data;
  }
}
?>
