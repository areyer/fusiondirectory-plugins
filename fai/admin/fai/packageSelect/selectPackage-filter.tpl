<div class="contentboxh">
 <p class="contentboxh">
  <img src="images/launch.png" align="right" alt="[F]">{t}Filter{/t}
 </p>
</div>

<div class="contentboxb">

<div style="border-top:1px solid #AAAAAA"></div>

 <div style='padding: 3px;'>
  {t}A maximum of 200 entries will be shown here.{/t}
 </div>

<div style='padding:4px;'>
{foreach from=$customs key=key item=value}
  {$value}
  <label for="{$key}">custom {$key}</label><br/>
{/foreach}
</div>

 <table summary="" style="width:100%;border-top:1px solid #B0B0B0;">
  <tr>
   <td>
    <label for="NAME">
     <img src="images/lists/search.png" align=middle>&nbsp;{t}Name{/t}
    </label>
   </td>
   <td>
    {$NAME}
   </td>
  </tr>
 </table>

 <table summary=""  width="100%"  style="background:#EEEEEE;border-top:1px solid #B0B0B0;">
  <tr>
   <td width="100%" align="right">
    {$APPLY}
   </td>
  </tr>
 </table>
</div>
