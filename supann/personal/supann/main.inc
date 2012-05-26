<?php
/*
 * This code is part of GOsa (http://www.gosa-project.org)
 * Copyright (C) 2012 FusionDirectory
 *
 * ID: $$Id: main.inc 15315 2010-01-26 13:00:52Z hickert $$
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* Remove locks created by this plugin
*/

$lock_msg = "";
if ($remove_lock || isset($_POST['edit_cancel'])) {
  if (session::is_set('supannAccount') && session::is_set('edit')) {
    del_lock($ui->dn);
  }
}

/* Remove this plugin from session
*/
if ($cleanup) {
  session::un_set('supannAccount');
  session::un_set('edit');
} else {
  /* Reset requested? */
  if (isset($_POST['edit_cancel'])) {
    session::un_set('edit');
    session::un_set('supannAccount');
  }

  /* Create supannAccount object on demand */
  if (!session::is_set('supannAccount') || (isset($_GET['reset']) && $_GET['reset'] == 1)) {
    $supannAccount = new supannAccount($config, $ui->dn);
    $supannAccount->enable_CSN_check();
    $supannAccount->set_acl_base($ui->dn);
    $supannAccount->set_acl_category("users");
    session::set('supannAccount', $supannAccount);
  }
  $supannAccount = session::get('supannAccount');

  /* save changes back to object */
  if (session::is_set('edit')) {
    $supannAccount->save_object();
  }

  /* Enter edit mode? */
  if ((isset($_POST['edit'])) && (!session::is_set('edit'))) {
    /* Check locking */
    if (($username = get_lock($ui->dn)) != "") {
      session::set('back_plugin', $plug);
      session::set('LOCK_VARS_TO_USE', array("/^edit$/", "/^plug$/"));
      $lock_msg = gen_locked_message($username, $ui->dn);
    } else {
      /* Lock the current entry */
      add_lock($ui->dn, $ui->dn);
      session::set('edit', TRUE);
    }
  }

  /* save changes to LDAP and disable edit mode */
  $info = "";
  if (isset($_POST['edit_finish'])) {
    /* Perform checks */
    $message = $supannAccount->check();

    /* No errors, save object */
    if (count($message) == 0) {
      $supannAccount->save();
      del_lock($ui->dn);
      session::un_set('edit');

      /* Remove from session */
      session::un_set('supannAccount');
    } else {
      /* Errors found, show message */
      msg_dialog::displayChecks($message);
    }
  }

  /* Execute formular */
  if ($lock_msg) {
    $display = $lock_msg;
  } else {
    $display .= $supannAccount->execute();
  }

  /* Store changes  in session */
  if (session::is_set('edit')) {
    session::set('supannAccount', $supannAccount);
  }

  /* Show page footer depending on the mode */
  if (!$supannAccount->dialog && $supannAccount->is_account && empty($lock_msg)) {
    $display .= "<p class=\"plugbottom\">";

    /* Are we in edit mode? */
    if (session::is_set('edit')) {
      $display .= "<input type=submit name=\"edit_finish\" style=\"width:80px\" value=\"".msgPool::okButton()."\">\n";
      $display .= "&nbsp;";
      $display .= "<input type=submit name=\"edit_cancel\" value=\"".msgPool::cancelButton()."\">\n";
      $info = "<img alt=\"\" align=\"middle\" src=\"".
              get_template_path('images/lists/locked.png')."\"> ".
              $ui->dn."&nbsp;";
    } else {
      $info = "<img alt=\"\" align=\"middle\" src=\"".
              get_template_path('images/lists/unlocked.png')."\"> ".
              $ui->dn."&nbsp;";

      /* Only display edit button if there is at least one attribute editable */
      if (preg_match("/r/", $ui->get_permissions($ui->dn, "users/supannAccount"))) {
        $info .=  "<img alt=\"\" align=\"middle\" src=\"".
                  get_template_path('images/lists/on.png')."\"> ".
                  msgPool::clickEditToChange();

        $display .= "<input type=submit name=\"edit\" value=\"".msgPool::editButton()."\">\n";
      }
      $display .= "<input type=\"hidden\" name=\"ignore\">\n";
    }
    $display .= "</p>\n";
  }

  /* Page header */
  $display = print_header(get_template_path('plugins/supann/images/plugin.png'),
                          _("SUPANN settings"), $info).$display;

}

?>