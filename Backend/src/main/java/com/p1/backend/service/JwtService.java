package com.p1.backend.service;

import io.jsonwebtoken.Claims;
import io.jsonwebtoken.Jwts;
import io.jsonwebtoken.security.Keys;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

import javax.crypto.SecretKey;
import java.nio.charset.StandardCharsets;
import java.util.Date;

// Spring Security 없이 JWT를 직접 발급/검증한다. 서명 실패/만료 시 io.jsonwebtoken.JwtException 계열을
// 그대로 던지므로, 호출부(JwtAuthInterceptor)가 이를 잡아 401로 변환한다.
@Service
public class JwtService {

    private final SecretKey signingKey;
    private final long expirationMs;

    public JwtService(@Value("${jwt.secret}") String secret,
                       @Value("${jwt.expiration-ms}") long expirationMs) {
        this.signingKey = Keys.hmacShaKeyFor(secret.getBytes(StandardCharsets.UTF_8));
        this.expirationMs = expirationMs;
    }

    // subject는 email — 로그인 식별자가 곧 JWT/매치메이킹 전반의 유일 식별자다(username은 닉네임일 뿐
    // 유일하지 않으므로 subject로 쓸 수 없음).
    public String generateToken(String email) {
        Date now = new Date();
        Date expiry = new Date(now.getTime() + expirationMs);
        return Jwts.builder()
                .subject(email)
                .issuedAt(now)
                .expiration(expiry)
                .signWith(signingKey)
                .compact();
    }

    public String parseEmail(String token) {
        Claims claims = Jwts.parser()
                .verifyWith(signingKey)
                .build()
                .parseSignedClaims(token)
                .getPayload();
        return claims.getSubject();
    }
}
