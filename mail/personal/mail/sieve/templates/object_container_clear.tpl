<table class='object_container_container'>
  <tr>
    <td class='object_container_cell_top_left'>
      &nbsp;
    </td>
    <td class='object_container_cell_top_right'>
        <input type='image' src='geticon.php?context=actions&icon=up&size=16' name='Move_Up_Object_{$ID}'
                title='{t}Move this object up one position{/t}' alt='{t}Up{/t}' class='center'>
            <a href='{$plug}&amp;Move_Up_Object_ID={$ID}'>{t}Move up{/t}</a>

            <input type='image' src='geticon.php?context=actions&icon=down&size=16' name='Move_Down_Object_{$ID}'
                title='{t}Move this object down one position{/t}' alt='{t}Down{/t}' class='center'>
            <a href='{$plug}&amp;Move_Down_Object_ID={$ID}'>{t}Move down{/t}</a>

            <input type='image' src='geticon.php?context=actions&icon=edit-delete&size=16' name='Remove_Object_{$ID}'
                title='{t}Remove this object{/t}' alt='R' class='center'>
            <a href='{$plug}&amp;Remove_Object_ID={$ID}'>{t}Remove element{/t}</a>

    </td>
    </tr>
    <tr>
    <td class='object_container_cell_bottom_left'>
        </td>
        <td>
            %%OBJECT_CONTENT%%
        </td>
    </tr>
</table>
