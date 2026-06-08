# Examples

Useful architecture decision snippets:

- Map `Task_Read_A_Row` to `Task_Generate_A_RowBlock`.
- Map `Task_Read_B_All` to `Task_Generate_B_Tile` or `Task_Generate_C_Tile`.
- Map `Task_Compute_All` to `Compute_AB_Tile` and `Consume_Tmp_Tile`.
- Map `Task_Write_C_All` to `Reduce_Sum` or optional D-store validation.

