<input type="hidden" name="mailedit" value="1">
<table style="width:100%; vertical-align:top; text-align:left;" cellpadding=0 border=0>

 <!-- Headline container -->
 <tr>
  <td style="width:50%; vertical-align:top;">
   <h2><img class="center" alt="" src="images/rightarrow.png"> {t}Mail distribution list{/t}</h2>
   <table>
    <tr>
     <td><LABEL for="mail">{t}Primary address{/t}</LABEL>{$must}</td>
     <td>
{render acl=$mailACL}
  <input type='text' id="mail" name="mail" size=50 maxlength=65 value="{$mail}" title="{t}Primary mail address for this distribution list{/t}">
{/render}
     </td>
    </tr>
   </table>
  </td>
 </tr>
</table>

<!-- Place cursor -->
<script type="text/javascript">
  <!-- // First input field on page
  focus_field('mail');
  -->
</script>
