<div class="contentboxh">
 <p class="contentboxh">
  <img src="images/launch.png" align="right" alt="[F]">{t}Filter{/t}
 </p>
</div>

<div class="contentboxb">

<div style='padding:4px;'>
  <LABEL for='RELEASE'>{t}Release{/t}</LABEL>&nbsp;{$RELEASE}<br>
</div>

<div style="display:block;width=100%;border-top:1px solid #AAAAAA;"></div>

<div style='padding:4px;'>

{if $fai_release != $fai_base && $allow_remove}
  <br>
  <input class="center" type="image" name="remove_branch" src="geticon.php?context=actions&icon=edit-delete&size=16">
  <a href="{$plug}&act=remove_branch">{t}Delete current release{/t}</a>
{/if}

</div>

<div style="display:block;width=100%;border-top:1px solid #AAAAAA;"></div>

{$PROFILE}<label for="PROFILE">   {t}Show profiles{/t}</label>
<br>{$TEMPLATE}<label for="TEMPLATE">  {t}Show templates{/t}</label>
<br>{$SCRIPT}<label for="SCRIPT">    {t}Show scripts{/t}</label>
<br>{$HOOK}<label for="HOOK">      {t}Show hooks{/t}</label>
<br>{$VARIABLE}<label for="VARIABLE">  {t}Show variables{/t}</label>
<br>{$PACKAGE}<label for="PACKAGE">   {t}Show packages{/t}</label>
<br>{$PARTITION}<label for="PARTITION"> {t}Show partitions{/t}</label>

 <table style="width:100%;border-top:1px solid #B0B0B0;">
  <tr>
   <td>
    <label for="NAME">
     <img src="geticon.php?context=actions&icon=system-search&size=16" align=middle>&nbsp;{t}Name{/t}
    </label>
   </td>
   <td>
    {$NAME}
   </td>
  </tr>
 </table>

 <table  width="100%"  style="background:#EEEEEE;border-top:1px solid #B0B0B0;">
  <tr>
   <td style="width:100%;text-align:right;">
    {$APPLY}
   </td>
  </tr>
 </table>
</div>
