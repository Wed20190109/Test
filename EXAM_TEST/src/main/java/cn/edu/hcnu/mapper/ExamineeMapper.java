package cn.edu.hcnu.mapper;

import cn.edu.hcnu.po.Examinee;
import lombok.Data;
import org.apache.ibatis.annotations.Delete;
import org.apache.ibatis.annotations.Insert;
import org.apache.ibatis.annotations.Select;
import org.apache.ibatis.annotations.Update;

import  java.util.List;

public interface ExamineeMapper {
    @Select("select * from examinee")
    List<Examinee> selectAllExaminee();
    @Delete("delete from examinee where id=#{id}")
   int deleteByPrimaryKey(Integer id);
    @Insert("insert into examinee(number, username, gender, phone, academy, major) " +
            "values(#{number}, #{username}, #{gender}, #{phone}, #{academy}, #{major})")
   int insert(Examinee record);
    @Update("update examinee set " +
            "number=#{number}, username=#{username}, gender=#{gender}, phone=#{phone}, academy=#{academy}, major=#{major} " +
            "where id=#{id}")
   int update(Examinee record);




}
