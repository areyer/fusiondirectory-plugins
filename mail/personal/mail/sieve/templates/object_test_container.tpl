<table class='sieve_test_container'>
  <tr>
    <td style='width:20px; ; text-align:center; vertical-align:top;'>
      {if $DisplayAdd}
        <input type='image' src='plugins/mail/images/sieve_add_test.png' name='Add_Test_Object_{$ID}'
          title='{t}Add object{/t}' alt='R' class='center'>
      {/if}
      {if $DisplayDel}
        <input type='image' src='geticon.php?context=actions&icon=edit-delete&size=16' name='Remove_Test_Object_{$ID}'
          title='{t}Remove object{/t}' alt='R' class='center'>
      {/if}
    </td>
    <td>
      %%OBJECT_CONTENT%%
    </td>
  </tr>
</table>
