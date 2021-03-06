<h2>{t}Phone settings{/t}</h2>
<table style="width:100%" cellspacing=0>
 <tr>
  <td style="vertical-align:top; border-right:1px solid #A0A0A0; width:50%">
   <h2>
    <img class="center" alt="" align="middle" src="plugins/asterisk/images/iconMini.png" />&nbsp;{t}Phone numbers{/t}
   </h2>
  </td>
 </tr>
 <tr>
  <td style="vertical-align:top; border-right:1px solid #A0A0A0; width:50%">
   <select style="width:100%;" name="phonenumber_list[]" size=7 multiple>
    {html_options options=$phoneNumbers}
     <option disabled>&nbsp; </option>
   </select>
   <br>
   <input type='text' name="phonenumber" size=20 align=middle maxlength=60 value="">
   <input type="submit" value="{msgPool type=addButton}" name="add_phonenumber">
   <input type="submit" value="{msgPool type=delButton}" name="delete_phonenumber">
  </td>
  <td style="vertical-align:top; width:50%;">
   <table style="width:100%" border=0>
    <tr>
     <td style="vertical-align:top;">
      <h2>
       <img class="center" alt="" align="middle" src="plugins/asterisk/images/hardware.png" />
       &nbsp;{t}Telephone hardware{/t}
      </h2>
      <table border=0>
       <tr>
        <td>
         <label for="goFonVoicemailPIN">{t}Voicemail PIN{/t}{$must}</label>
        </td>
        <td>
         <input type="password" id="goFonVoicemailPIN" name="goFonVoicemailPI" value="{$goFonVoicemailPIN}">
        </td>
       </tr>
       <tr>
        <td>
         <label for="goFonPIN">{t}Phone PIN{/t}{$must}</label>
        </td>
        <td>
         <input type="password" id="goFonPIN" name="goFonPIN" value="{$goFonPIN}">
        </td>
       </tr>
      </table>
     </td>
    </tr>
   </table>
  </td>
 </tr>
</table>
<input type="hidden" name="phoneTab" value="phoneTab">
<br>
<br>
